#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <limits>
#include <type_traits>

#include "arc/place.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::dsp {

template <typename T>
struct Phase3 {
    T a{};
    T b{};
    T c{};
};

template <typename T>
struct AlphaBeta {
    T alpha{};
    T beta{};
};

template <typename T>
struct Dq {
    T d{};
    T q{};
};

template <typename T>
[[nodiscard]] constexpr T clamp(const T value, const T lo, const T hi) noexcept
{
    return value < lo ? lo : (value > hi ? hi : value);
}

template <typename T>
[[nodiscard]] constexpr AlphaBeta<T> clarke(const Phase3<T> phase) noexcept
{
    return {
        .alpha = phase.a,
        .beta = (phase.a + (phase.b * T{2})) * T{0.5773502691896258},
    };
}

template <typename T>
[[nodiscard]] constexpr Phase3<T> inverse_clarke(const AlphaBeta<T> value) noexcept
{
    return {
        .a = value.alpha,
        .b = (T{-0.5} * value.alpha) + (T{0.8660254037844386} * value.beta),
        .c = (T{-0.5} * value.alpha) - (T{0.8660254037844386} * value.beta),
    };
}

template <typename T>
[[nodiscard]] constexpr Dq<T> park(
    const AlphaBeta<T> value,
    const T sin_theta,
    const T cos_theta) noexcept
{
    return {
        .d = (value.alpha * cos_theta) + (value.beta * sin_theta),
        .q = (value.beta * cos_theta) - (value.alpha * sin_theta),
    };
}

template <typename T>
[[nodiscard]] constexpr AlphaBeta<T> inverse_park(
    const Dq<T> value,
    const T sin_theta,
    const T cos_theta) noexcept
{
    return {
        .alpha = (value.d * cos_theta) - (value.q * sin_theta),
        .beta = (value.d * sin_theta) + (value.q * cos_theta),
    };
}

template <typename T>
[[nodiscard]] constexpr Phase3<T> duty_centered(
    const AlphaBeta<T> voltage,
    const T bus_voltage) noexcept
{
    if (bus_voltage <= T{}) {
        return {T{0.5}, T{0.5}, T{0.5}};
    }

    const auto phase = inverse_clarke(voltage);
    const auto scale = T{0.5} / bus_voltage;
    return {
        .a = clamp(T{0.5} + (phase.a * scale), T{}, T{1}),
        .b = clamp(T{0.5} + (phase.b * scale), T{}, T{1}),
        .c = clamp(T{0.5} + (phase.c * scale), T{}, T{1}),
    };
}

template <typename T>
[[nodiscard]] constexpr Phase3<T> duty_svpwm(
    const AlphaBeta<T> voltage,
    const T bus_voltage) noexcept
{
    if (bus_voltage <= T{}) {
        return {T{0.5}, T{0.5}, T{0.5}};
    }

    auto phase = inverse_clarke(voltage);
    const auto min_v = phase.a < phase.b ? (phase.a < phase.c ? phase.a : phase.c) : (phase.b < phase.c ? phase.b : phase.c);
    const auto max_v = phase.a > phase.b ? (phase.a > phase.c ? phase.a : phase.c) : (phase.b > phase.c ? phase.b : phase.c);
    const auto offset = (max_v + min_v) * T{-0.5};
    const auto scale = T{0.5} / bus_voltage;
    return {
        .a = clamp(T{0.5} + ((phase.a + offset) * scale), T{}, T{1}),
        .b = clamp(T{0.5} + ((phase.b + offset) * scale), T{}, T{1}),
        .c = clamp(T{0.5} + ((phase.c + offset) * scale), T{}, T{1}),
    };
}

template <typename T>
struct PllObserver {
    T angle{};
    T velocity{};

    ARC_HOT void step(
        const T measured_angle,
        const T kp,
        const T ki,
        const T dt) noexcept
    {
        auto error = measured_angle - angle;
        while (error > T{3.141592653589793}) {
            error -= T{6.283185307179586};
        }
        while (error < T{-3.141592653589793}) {
            error += T{6.283185307179586};
        }
        velocity += (ki * error * dt);
        angle += ((velocity + (kp * error)) * dt);
        while (angle >= T{6.283185307179586}) {
            angle -= T{6.283185307179586};
        }
        while (angle < T{}) {
            angle += T{6.283185307179586};
        }
    }
};

template <typename T>
struct SlidingModeObserver {
    T estimated_alpha{};
    T estimated_beta{};

    ARC_HOT void step(
        const AlphaBeta<T> current,
        const AlphaBeta<T> voltage,
        const T gain,
        const T dt) noexcept
    {
        const auto err_a = current.alpha - estimated_alpha;
        const auto err_b = current.beta - estimated_beta;
        estimated_alpha += ((voltage.alpha + clamp(err_a * gain, T{-1}, T{1})) * dt);
        estimated_beta += ((voltage.beta + clamp(err_b * gain, T{-1}, T{1})) * dt);
    }
};

struct ScalarDsp {
    [[nodiscard]] ARC_HOT static inline float dot_f32(
        const float* __restrict lhs,
        const float* __restrict rhs,
        const std::size_t count) noexcept
    {
        float acc{};
#pragma GCC ivdep
        for (std::size_t i = 0; i < count; ++i) {
            acc += lhs[i] * rhs[i];
        }
        return acc;
    }

    ARC_HOT static inline void mac_f32(
        float* __restrict acc,
        const float* __restrict in,
        const float gain,
        const std::size_t count) noexcept
    {
#pragma GCC ivdep
        for (std::size_t i = 0; i < count; ++i) {
            acc[i] += in[i] * gain;
        }
    }
};

template <typename Backend = ScalarDsp>
struct DspAccel {
    [[nodiscard]] ARC_HOT static inline float dot_f32(
        const std::span<const float> lhs,
        const std::span<const float> rhs) noexcept
    {
        const auto count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
        return Backend::dot_f32(lhs.data(), rhs.data(), count);
    }

    ARC_HOT static inline void mac_f32(
        const std::span<float> acc,
        const std::span<const float> in,
        const float gain) noexcept
    {
        const auto count = acc.size() < in.size() ? acc.size() : in.size();
        Backend::mac_f32(acc.data(), in.data(), gain, count);
    }
};

template <typename T>
[[nodiscard]] ARC_HOT inline T dot(
    const T* __restrict lhs,
    const T* __restrict rhs,
    const std::size_t count) noexcept
{
    T acc{};
#pragma GCC ivdep
    for (std::size_t i = 0; i < count; ++i) {
        acc += lhs[i] * rhs[i];
    }
    return acc;
}

template <typename T>
ARC_HOT inline void scale(
    T* __restrict out,
    const T* __restrict in,
    const T gain,
    const std::size_t count) noexcept
{
#pragma GCC ivdep
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = in[i] * gain;
    }
}

template <typename T>
ARC_HOT inline void mix(
    T* __restrict out,
    const T* __restrict lhs,
    const T* __restrict rhs,
    const std::size_t count) noexcept
{
#pragma GCC ivdep
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = lhs[i] + rhs[i];
    }
}

template <typename T>
ARC_HOT inline void mac(
    T* __restrict acc,
    const T* __restrict in,
    const T gain,
    const std::size_t count) noexcept
{
#pragma GCC ivdep
    for (std::size_t i = 0; i < count; ++i) {
        acc[i] += in[i] * gain;
    }
}

template <typename T>
[[nodiscard]] ARC_HOT inline T peak(
    const T* __restrict in,
    const std::size_t count) noexcept
{
    T top{};
    for (std::size_t i = 0; i < count; ++i) {
        const auto value = in[i] < T{} ? -in[i] : in[i];
        top = value > top ? value : top;
    }
    return top;
}

template <typename T, std::size_t Taps>
struct Fir {
    static_assert(Taps > 0U, "FIR tap count must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "FIR works on arithmetic sample types");

    struct State {
        std::array<T, Taps> hist{};
        std::size_t head{};
    };

    using Coeffs = std::array<T, Taps>;

    static void clear(State& state) noexcept
    {
        state.hist.fill(T{});
        state.head = 0U;
    }

    [[nodiscard]] ARC_HOT static T step(
        State& state,
        const Coeffs& taps,
        const T sample) noexcept
    {
        state.head = state.head == 0U ? Taps - 1U : state.head - 1U;
        state.hist[state.head] = sample;

        T acc{};
        for (std::size_t i = 0; i < Taps; ++i) {
            const auto slot = state.head + i < Taps ? state.head + i : state.head + i - Taps;
            acc += state.hist[slot] * taps[i];
        }
        return acc;
    }

    ARC_HOT static void run(
        T* __restrict out,
        State& state,
        const Coeffs& taps,
        const T* __restrict in,
        const std::size_t count) noexcept
    {
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = step(state, taps, in[i]);
        }
    }
};

template <typename T>
struct Pid {
    static_assert(std::is_arithmetic_v<T>, "PID works on arithmetic sample types");

    struct Gains {
        T kp{};
        T ki{};
        T kd{};
    };

    struct Limits {
        T out_min{std::numeric_limits<T>::lowest()};
        T out_max{std::numeric_limits<T>::max()};
        T i_min{std::numeric_limits<T>::lowest()};
        T i_max{std::numeric_limits<T>::max()};
    };

    struct State {
        T integral{};
        T previous_error{};
        T previous_measurement{};
        bool primed{};
    };

    static void clear(State& state) noexcept
    {
        state = {};
    }

    [[nodiscard]] ARC_HOT static T step(
        State& state,
        const Gains gains,
        const Limits limits,
        const T setpoint,
        const T measurement,
        const T dt) noexcept
    {
        if (dt <= T{}) {
            return clamp(gains.kp * (setpoint - measurement), limits.out_min, limits.out_max);
        }

        const auto error = setpoint - measurement;
        state.integral = clamp(state.integral + (error * dt), limits.i_min, limits.i_max);

        const auto derivative = state.primed ? (measurement - state.previous_measurement) / dt : T{};
        state.previous_error = error;
        state.previous_measurement = measurement;
        state.primed = true;

        return clamp(
            (gains.kp * error) + (gains.ki * state.integral) - (gains.kd * derivative),
            limits.out_min,
            limits.out_max);
    }
};

template <typename T>
struct Biquad {
    static_assert(std::is_arithmetic_v<T>, "Biquad works on arithmetic sample types");

    struct Coeffs {
        T b0{};
        T b1{};
        T b2{};
        T a1{};
        T a2{};
    };

    struct State {
        T x1{};
        T x2{};
        T y1{};
        T y2{};
    };

    static void clear(State& state) noexcept
    {
        state = {};
    }

    [[nodiscard]] ARC_HOT static T step(
        State& state,
        const Coeffs coeffs,
        const T sample) noexcept
    {
        const auto out = (coeffs.b0 * sample) + (coeffs.b1 * state.x1) + (coeffs.b2 * state.x2) - (coeffs.a1 * state.y1) - (coeffs.a2 * state.y2);
        state.x2 = state.x1;
        state.x1 = sample;
        state.y2 = state.y1;
        state.y1 = out;
        return out;
    }

    ARC_HOT static void run(
        T* __restrict out,
        State& state,
        const Coeffs coeffs,
        const T* __restrict in,
        const std::size_t count) noexcept
    {
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = step(state, coeffs, in[i]);
        }
    }
};

template <typename T, std::size_t Sections>
struct Sos {
    static_assert(Sections > 0U, "SOS section count must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "SOS works on arithmetic sample types");

    using Section = Biquad<T>;
    using Coeffs = std::array<typename Section::Coeffs, Sections>;

    struct State {
        std::array<typename Section::State, Sections> sections{};
    };

    static void clear(State& state) noexcept
    {
        for (auto& section : state.sections) {
            Section::clear(section);
        }
    }

    [[nodiscard]] ARC_HOT static T step(
        State& state,
        const Coeffs& coeffs,
        const T sample) noexcept
    {
        auto value = sample;
        for (std::size_t i = 0; i < Sections; ++i) {
            value = Section::step(state.sections[i], coeffs[i], value);
        }
        return value;
    }

    ARC_HOT static void run(
        T* __restrict out,
        State& state,
        const Coeffs& coeffs,
        const T* __restrict in,
        const std::size_t count) noexcept
    {
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = step(state, coeffs, in[i]);
        }
    }
};

template <typename T>
struct BeamEstimate {
    std::int32_t lag_samples{};
    T confidence{};
    T angle_rad{};
};

template <typename T, std::size_t Channels>
struct Beamform {
    static_assert(Channels > 0U, "Beamform needs at least one channel");
    static_assert(std::is_arithmetic_v<T>, "Beamform works on arithmetic sample types");

    using Inputs = std::array<std::span<const T>, Channels>;

    struct DelaySumPlan {
        std::array<std::size_t, Channels> delay_samples{};
        std::array<T, Channels> gain{};
        T scale{T{1}};
    };

    [[nodiscard]] ARC_HOT static Result<std::size_t> delay_sum(
        const Inputs& inputs,
        const DelaySumPlan& plan,
        const std::span<T> out) noexcept
    {
        if (out.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (std::size_t channel = 0; channel < Channels; ++channel) {
            if (inputs[channel].empty() || plan.delay_samples[channel] >= inputs[channel].size()) {
                return fail(ESP_ERR_INVALID_ARG);
            }
        }

        auto produced = std::size_t{};
        for (; produced < out.size(); ++produced) {
            T acc{};
            for (std::size_t channel = 0; channel < Channels; ++channel) {
                const auto index = produced + plan.delay_samples[channel];
                if (index >= inputs[channel].size()) {
                    return produced;
                }
                acc += inputs[channel][index] * plan.gain[channel];
            }
            out[produced] = acc * plan.scale;
        }
        return produced;
    }

    [[nodiscard]] ARC_HOT static Result<BeamEstimate<T>> lag_xcorr(
        const std::span<const T> reference,
        const std::span<const T> delayed,
        const std::size_t max_lag) noexcept
    {
        constexpr auto max_scan_lag = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() - 1);
        if (reference.empty() || delayed.empty() || max_lag == 0U || max_lag > max_scan_lag) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        BeamEstimate<T> best{};
        auto best_ready = false;
        const auto signed_max = static_cast<std::int32_t>(max_lag);
        for (auto lag = -signed_max; lag <= signed_max; ++lag) {
            T acc{};
            auto count = std::size_t{};
            for (std::size_t i = 0; i < reference.size(); ++i) {
                const auto j_signed = static_cast<std::int64_t>(i) + lag;
                if (j_signed < 0 || static_cast<std::uint64_t>(j_signed) >= delayed.size()) {
                    continue;
                }
                acc += reference[i] * delayed[static_cast<std::size_t>(j_signed)];
                ++count;
            }
            if (count == 0U) {
                continue;
            }
            const auto score = acc < T{} ? -acc : acc;
            if (!best_ready || score > best.confidence) {
                best = {
                    .lag_samples = lag,
                    .confidence = score,
                    .angle_rad = {},
                };
                best_ready = true;
            }
        }
        return best_ready ? Result<BeamEstimate<T>>{best} : fail(ESP_ERR_INVALID_STATE);
    }

    [[nodiscard]] ARC_HOT static Result<BeamEstimate<float>> gcc_phat(
        const std::span<simd::ComplexF32> reference,
        const std::span<simd::ComplexF32> delayed,
        const std::span<simd::ComplexF32> scratch,
        const std::size_t max_lag,
        const float sample_hz,
        const float mic_spacing_m,
        const float sound_mps = 343.0F) noexcept
    {
        if (reference.size() != delayed.size() || scratch.size() < reference.size() ||
            !simd::is_pow2(reference.size()) || max_lag == 0U ||
            sample_hz <= 0.0F || mic_spacing_m <= 0.0F || sound_mps <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto status = simd::fft_radix2(reference);
        if (!status) {
            return fail(status.error());
        }
        status = simd::fft_radix2(delayed);
        if (!status) {
            return fail(status.error());
        }

        for (std::size_t i = 0; i < reference.size(); ++i) {
            const simd::ComplexF32 conj_delayed{
                .re = delayed[i].re,
                .im = -delayed[i].im,
            };
            const auto cross = reference[i] * conj_delayed;
            const auto mag = std::sqrt((cross.re * cross.re) + (cross.im * cross.im));
            scratch[i] = mag > 0.0F ? simd::ComplexF32{.re = cross.re / mag, .im = cross.im / mag} : simd::ComplexF32{};
        }

        status = simd::fft_radix2(scratch.first(reference.size()), true);
        if (!status) {
            return fail(status.error());
        }

        BeamEstimate<float> best{};
        auto best_ready = false;
        const auto bounded_lag = max_lag < (reference.size() / 2U) ? max_lag : (reference.size() / 2U);
        constexpr auto max_scan_lag = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() - 1);
        if (bounded_lag > max_scan_lag) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto signed_max = static_cast<std::int32_t>(bounded_lag);
        for (auto lag = -signed_max; lag <= signed_max; ++lag) {
            const auto index = lag < 0 ? reference.size() - static_cast<std::size_t>(-lag) : static_cast<std::size_t>(lag);
            const auto score = scratch[index].re < 0.0F ? -scratch[index].re : scratch[index].re;
            if (!best_ready || score > best.confidence) {
                const auto delay_s = static_cast<float>(lag) / sample_hz;
                const auto ratio = clamp((delay_s * sound_mps) / mic_spacing_m, -1.0F, 1.0F);
                best = {
                    .lag_samples = lag,
                    .confidence = score,
                    .angle_rad = std::asin(ratio),
                };
                best_ready = true;
            }
        }
        return best_ready ? Result<BeamEstimate<float>>{best} : fail(ESP_ERR_INVALID_STATE);
    }
};

template <typename T, std::size_t Taps>
struct Aec {
    static_assert(Taps > 0U, "AEC tap count must be non-zero");
    static_assert(std::is_floating_point_v<T>, "AEC works on floating-point sample types");

    struct Config {
        T mu{T{0.25}};
        T epsilon{static_cast<T>(0.000001)};
        T leak{T{1}};
    };

    struct State {
        std::array<T, Taps> taps{};
        std::array<T, Taps> history{};
        std::size_t head{};
    };

    static void clear(State& state) noexcept
    {
        state = {};
    }

    [[nodiscard]] ARC_HOT static T step(
        State& state,
        const T far_end,
        const T microphone,
        const Config config = {}) noexcept
    {
        state.head = state.head == 0U ? Taps - 1U : state.head - 1U;
        state.history[state.head] = far_end;

        T echo{};
        T power{config.epsilon};
        for (std::size_t i = 0; i < Taps; ++i) {
            const auto slot = state.head + i < Taps ? state.head + i : state.head + i - Taps;
            const auto sample = state.history[slot];
            echo += state.taps[i] * sample;
            power += sample * sample;
        }

        const auto error = microphone - echo;
        const auto adapt = power != T{} ? (config.mu * error) / power : T{};
        for (std::size_t i = 0; i < Taps; ++i) {
            const auto slot = state.head + i < Taps ? state.head + i : state.head + i - Taps;
            state.taps[i] = (state.taps[i] * config.leak) + (adapt * state.history[slot]);
        }
        return error;
    }

    [[nodiscard]] ARC_HOT static Result<std::size_t> run(
        const std::span<T> out,
        State& state,
        const std::span<const T> far_end,
        const std::span<const T> microphone,
        const Config config = {}) noexcept
    {
        const auto count = out.size() < far_end.size() ? (out.size() < microphone.size() ? out.size() : microphone.size()) : (far_end.size() < microphone.size() ? far_end.size() : microphone.size());
        if (count == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = step(state, far_end[i], microphone[i], config);
        }
        return count;
    }
};

template <typename T, std::size_t States, std::size_t Inputs, std::size_t Outputs>
struct StateSpace {
    static_assert(States > 0U, "StateSpace state count must be non-zero");
    static_assert(Inputs > 0U, "StateSpace input count must be non-zero");
    static_assert(Outputs > 0U, "StateSpace output count must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "StateSpace works on arithmetic sample types");

    using StateVec = std::array<T, States>;
    using InputVec = std::array<T, Inputs>;
    using OutputVec = std::array<T, Outputs>;
    using A = std::array<std::array<T, States>, States>;
    using B = std::array<std::array<T, Inputs>, States>;
    using C = std::array<std::array<T, States>, Outputs>;
    using D = std::array<std::array<T, Inputs>, Outputs>;

    struct Model {
        A a{};
        B b{};
        C c{};
        D d{};
    };

    struct State {
        StateVec x{};
        StateVec next{};
    };

    static void clear(State& state) noexcept
    {
        state = {};
    }

    [[nodiscard]] ARC_HOT static OutputVec step(
        State& state,
        const Model& model,
        const InputVec& input) noexcept
    {
        OutputVec output{};

        for (std::size_t row = 0; row < Outputs; ++row) {
            T acc{};
            for (std::size_t col = 0; col < States; ++col) {
                acc += model.c[row][col] * state.x[col];
            }
            for (std::size_t col = 0; col < Inputs; ++col) {
                acc += model.d[row][col] * input[col];
            }
            output[row] = acc;
        }

        for (std::size_t row = 0; row < States; ++row) {
            T acc{};
            for (std::size_t col = 0; col < States; ++col) {
                acc += model.a[row][col] * state.x[col];
            }
            for (std::size_t col = 0; col < Inputs; ++col) {
                acc += model.b[row][col] * input[col];
            }
            state.next[row] = acc;
        }

        state.x = state.next;
        return output;
    }
};

}  // namespace arc::dsp
