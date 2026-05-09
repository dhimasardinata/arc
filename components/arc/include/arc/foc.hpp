#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "arc/dsp.hpp"
#include "arc/result.hpp"
#include "arc/rcu.hpp"

namespace arc {

template <typename T>
struct FocTarget {
    T d{};
    T q{};
    T bus{};
};

template <typename T>
struct FocSample {
    dsp::Phase3<T> current{};
    T sin_theta{};
    T cos_theta{};
};

template <typename T>
struct FocOutput {
    dsp::Dq<T> current{};
    dsp::AlphaBeta<T> voltage{};
    dsp::Phase3<T> duty{};
};

template <typename T>
struct FocConfig {
    dsp::Pid<T>::Gains d_gains{};
    dsp::Pid<T>::Gains q_gains{};
    dsp::Pid<T>::Limits limits{};
    bool svpwm{true};
};

template <typename T>
struct FocState {
    typename dsp::Pid<T>::State d{};
    typename dsp::Pid<T>::State q{};
};

template <typename T>
struct FocMath {
    static_assert(std::is_arithmetic_v<T>, "FOC scalar must be arithmetic");

    [[nodiscard]] ARC_HOT static FocOutput<T> step(
        FocState<T>& state,
        const FocConfig<T>& config,
        const FocTarget<T>& target,
        const FocSample<T>& sample,
        const T dt) noexcept
    {
        const auto alpha_beta = dsp::clarke(sample.current);
        const auto current = dsp::park(alpha_beta, sample.sin_theta, sample.cos_theta);
        const dsp::Dq voltage{
            .d = dsp::Pid<T>::step(state.d, config.d_gains, config.limits, target.d, current.d, dt),
            .q = dsp::Pid<T>::step(state.q, config.q_gains, config.limits, target.q, current.q, dt),
        };
        const auto output = dsp::inverse_park(voltage, sample.sin_theta, sample.cos_theta);
        return {
            .current = current,
            .voltage = output,
            .duty = config.svpwm ? dsp::duty_svpwm(output, target.bus) : dsp::duty_centered(output, target.bus),
        };
    }
};

template <typename Bridge, typename Scope, typename Encoder, typename T>
struct Foc {
    using scalar = T;
    using Target = FocTarget<T>;
    using Sample = FocSample<T>;
    using Output = FocOutput<T>;
    using Config = FocConfig<T>;
    using State = FocState<T>;

    Rcu<Target> target{};
    Config config{};
    State state{};

    [[nodiscard]] ARC_HOT Output step(const Sample& sample, const T dt) noexcept
    {
        static_cast<void>(sizeof(Bridge));
        static_cast<void>(sizeof(Scope));
        static_cast<void>(sizeof(Encoder));
        return FocMath<T>::step(state, config, target.read(), sample, dt);
    }
};

template <typename T>
struct DualFocSample {
    FocSample<T> a{};
    FocSample<T> b{};
};

template <typename T>
struct DualFocOutput {
    FocOutput<T> a{};
    FocOutput<T> b{};
};

template <typename MotorA, typename MotorB>
struct DualFoc {
    static_assert(std::is_same_v<typename MotorA::scalar, typename MotorB::scalar>, "dual FOC motors must use the same scalar");

    using scalar = typename MotorA::scalar;
    using Sample = DualFocSample<scalar>;
    using Output = DualFocOutput<scalar>;

    MotorA a{};
    MotorB b{};

    [[nodiscard]] ARC_HOT Output step(const Sample& sample, const scalar dt) noexcept
    {
        return {
            .a = a.step(sample.a, dt),
            .b = b.step(sample.b, dt),
        };
    }
};

enum class FocCarrierSample : std::uint8_t {
    empty,
    peak,
    center,
};

struct FocSyncPlan {
    std::uint32_t carrier_hz{};
    std::uint32_t phase_ticks{};
    FocCarrierSample current_sample{FocCarrierSample::center};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return carrier_hz != 0U;
    }
};

template <typename... Bridges>
struct BridgeCarrierSync {
    static_assert(sizeof...(Bridges) > 1U, "carrier sync needs at least two bridges");

    [[nodiscard]] static constexpr bool same_frequency() noexcept
    {
        const std::array<std::uint32_t, sizeof...(Bridges)> hz{Bridges::frequency()...};
        for (std::size_t i = 1; i < hz.size(); ++i) {
            if (hz[i] != hz[0]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr FocSyncPlan plan(
        const FocCarrierSample sample = FocCarrierSample::center,
        const std::uint32_t phase_ticks = 0U) noexcept
    {
        const std::array<std::uint32_t, sizeof...(Bridges)> hz{Bridges::frequency()...};
        return {
            .carrier_hz = same_frequency() ? hz[0] : 0U,
            .phase_ticks = phase_ticks,
            .current_sample = sample,
        };
    }
};

template <typename Bridge, typename AdcTask>
struct FocEtmCurrentTrigger {
    using bridge = Bridge;
    using adc_task = AdcTask;

    FocCarrierSample sample{FocCarrierSample::center};
    std::uint32_t phase_ticks{};

    [[nodiscard]] constexpr FocSyncPlan plan() const noexcept
    {
        return {
            .carrier_hz = Bridge::frequency(),
            .phase_ticks = phase_ticks,
            .current_sample = sample,
        };
    }
};

template <typename T>
struct FocEncoderSample {
    T absolute_angle{};
    T incremental_angle{};
    T velocity{};
    bool absolute_valid{};
};

template <typename SpiEncoder, typename QuadratureEncoder, typename KalmanFilter, typename T>
struct FocEncoderFusion {
    using spi_encoder = SpiEncoder;
    using quadrature_encoder = QuadratureEncoder;
    using filter_type = KalmanFilter;
    using scalar = T;

    KalmanFilter filter{};
    T angle{};
    T velocity{};

    [[nodiscard]] constexpr T step(const FocEncoderSample<T> sample) noexcept
    {
        const auto measured = sample.absolute_valid ? sample.absolute_angle : sample.incremental_angle;
        angle = measured;
        velocity = sample.velocity;
        if constexpr (requires { filter.x(0, 0) = measured; }) {
            filter.x(0, 0) = measured;
        }
        if constexpr (requires { filter.x(1, 0) = sample.velocity; }) {
            filter.x(1, 0) = sample.velocity;
        }
        return angle;
    }
};

}  // namespace arc
