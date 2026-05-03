#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <limits>
#include <type_traits>

#include "arc/place.hpp"

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
