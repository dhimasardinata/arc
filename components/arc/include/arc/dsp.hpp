#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "arc/place.hpp"

namespace arc::dsp {

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

}  // namespace arc::dsp
