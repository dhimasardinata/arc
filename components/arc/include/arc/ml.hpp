#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::ml {

template <typename T, std::size_t... Dims>
struct Tensor {
    static_assert(sizeof...(Dims) > 0U, "tensor needs at least one dimension");
    static_assert(((Dims > 0U) && ...), "tensor dimensions must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "tensor element must be arithmetic");

    using value_type = T;
    static constexpr auto rank = sizeof...(Dims);
    static constexpr std::array<std::size_t, rank> shape{Dims...};
    static constexpr std::size_t size = (Dims * ...);

    std::array<T, size> values{};

    [[nodiscard]] constexpr T* data() noexcept
    {
        return values.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept
    {
        return values.data();
    }

    [[nodiscard]] constexpr std::span<T, size> span() noexcept
    {
        return std::span<T, size>{values};
    }

    [[nodiscard]] constexpr std::span<const T, size> span() const noexcept
    {
        return std::span<const T, size>{values};
    }

    [[nodiscard]] constexpr T& operator[](const std::size_t index) noexcept
    {
        return values[index];
    }

    [[nodiscard]] constexpr const T& operator[](const std::size_t index) const noexcept
    {
        return values[index];
    }
};

template <typename T, std::size_t... Dims>
inline constexpr std::array<std::size_t, Tensor<T, Dims...>::rank> Tensor<T, Dims...>::shape;

template <typename T, std::size_t N>
[[nodiscard]] constexpr T dot(
    const std::span<const T, N> lhs,
    const std::span<const T, N> rhs) noexcept
{
    T acc{};
    for (std::size_t i = 0; i < N; ++i) {
        acc = static_cast<T>(acc + (lhs[i] * rhs[i]));
    }
    return acc;
}

template <std::size_t In, std::size_t Out, typename T = float>
struct Dense {
    static_assert(In > 0U && Out > 0U, "dense layer dimensions must be non-zero");

    std::span<const T, In * Out> weights{};
    std::span<const T, Out> bias{};

    [[nodiscard]] Status eval(
        const std::span<const T, In> input,
        const std::span<T, Out> output) const noexcept
    {
        if (weights.size() != In * Out || bias.size() != Out) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t row = 0; row < Out; ++row) {
            auto acc = bias[row];
            const auto base = row * In;
            for (std::size_t col = 0; col < In; ++col) {
                acc = static_cast<T>(acc + (weights[base + col] * input[col]));
            }
            output[row] = acc;
        }
        return ok();
    }
};

template <std::size_t In, std::size_t Out>
struct QuantDenseS8 {
    static_assert(In > 0U && Out > 0U, "quant dense dimensions must be non-zero");

    std::span<const std::int8_t, In * Out> weights{};
    std::span<const std::int32_t, Out> bias{};
    std::int32_t input_zero{};
    std::int32_t weight_zero{};
    std::int32_t output_zero{};
    std::int32_t multiplier{1};
    std::uint8_t shift{};

    [[nodiscard]] Status eval(
        const std::span<const std::int8_t, In> input,
        const std::span<std::int8_t, Out> output) const noexcept
    {
        if (weights.size() != In * Out || bias.size() != Out || shift >= 31U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t row = 0; row < Out; ++row) {
            auto acc = bias[row];
            const auto base = row * In;
            for (std::size_t col = 0; col < In; ++col) {
                const auto x = static_cast<std::int32_t>(input[col]) - input_zero;
                const auto w = static_cast<std::int32_t>(weights[base + col]) - weight_zero;
                acc += x * w;
            }

            const auto scaled = (static_cast<std::int64_t>(acc) * multiplier) >> shift;
            const auto shifted = scaled + output_zero;
            output[row] = saturate_s8(shifted);
        }
        return ok();
    }

private:
    [[nodiscard]] static constexpr std::int8_t saturate_s8(const std::int64_t value) noexcept
    {
        if (value > std::numeric_limits<std::int8_t>::max()) {
            return std::numeric_limits<std::int8_t>::max();
        }
        if (value < std::numeric_limits<std::int8_t>::min()) {
            return std::numeric_limits<std::int8_t>::min();
        }
        return static_cast<std::int8_t>(value);
    }
};

template <typename T, std::size_t N>
void relu(const std::span<T, N> values) noexcept
{
    for (auto& value : values) {
        if (value < T{}) {
            value = T{};
        }
    }
}

template <typename T, std::size_t N>
[[nodiscard]] std::size_t argmax(const std::span<const T, N> values) noexcept
{
    static_assert(N > 0U, "argmax needs at least one value");
    auto best = std::size_t{};
    for (std::size_t i = 1; i < N; ++i) {
        if (values[i] > values[best]) {
            best = i;
        }
    }
    return best;
}

}  // namespace arc::ml
