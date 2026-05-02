#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "arc/place.hpp"

namespace arc::dsp {

template <typename T>
[[nodiscard]] constexpr T clamp(const T value, const T lo, const T hi) noexcept
{
    return value < lo ? lo : (value > hi ? hi : value);
}

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

}  // namespace arc::dsp
