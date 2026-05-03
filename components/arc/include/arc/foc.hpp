#pragma once

#include <cstdint>
#include <type_traits>

#include "arc/dsp.hpp"
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
            .duty = dsp::duty_centered(output, target.bus),
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

}  // namespace arc
