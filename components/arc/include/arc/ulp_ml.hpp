#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/ulp_cxx.hpp"

namespace arc::ulp::ml {

struct QuantParams {
    std::int32_t input_zero{};
    std::int32_t weight_zero{};
    std::int32_t output_zero{};
    std::int32_t multiplier{1};
    std::uint8_t shift{};
};

[[nodiscard]] constexpr std::int8_t saturate_s8(const std::int64_t value) noexcept
{
    return value > 127 ? 127 : (value < -128 ? -128 : static_cast<std::int8_t>(value));
}

[[nodiscard]] constexpr std::int8_t requantize(
    const std::int64_t acc,
    const QuantParams params) noexcept
{
    const auto scaled = params.shift >= 31U ? 0 : ((acc * params.multiplier) >> params.shift);
    return saturate_s8(scaled + params.output_zero);
}

template <std::size_t In, std::size_t Out>
struct QuantDenseS8 {
    static_assert(In > 0U && Out > 0U, "ULP dense dimensions must be non-zero");

    const std::int8_t* weights{};
    const std::int32_t* bias{};
    QuantParams params{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return weights != nullptr && bias != nullptr && params.shift < 31U;
    }

    [[nodiscard]] constexpr bool eval(
        const std::int8_t* const input,
        std::int8_t* const output) const noexcept
    {
        if (input == nullptr || output == nullptr || !valid()) {
            return false;
        }

        for (std::size_t row = 0; row < Out; ++row) {
            auto acc = static_cast<std::int64_t>(bias[row]);
            const auto base = row * In;
            for (std::size_t col = 0; col < In; ++col) {
                acc += (static_cast<std::int32_t>(input[col]) - params.input_zero) *
                    (static_cast<std::int32_t>(weights[base + col]) - params.weight_zero);
            }
            output[row] = requantize(acc, params);
        }
        return true;
    }

    [[nodiscard]] constexpr bool eval(
        const std::span<const std::int8_t, In> input,
        const std::span<std::int8_t, Out> output) const noexcept
    {
        return eval(input.data(), output.data());
    }
};

template <std::size_t Bytes>
[[nodiscard]] constexpr std::array<std::int8_t, Bytes> center_u8(
    const std::array<std::uint8_t, Bytes>& input,
    const std::uint8_t zero = 128U) noexcept
{
    std::array<std::int8_t, Bytes> out{};
    for (std::size_t i = 0; i < Bytes; ++i) {
        out[i] = saturate_s8(static_cast<std::int32_t>(input[i]) - static_cast<std::int32_t>(zero));
    }
    return out;
}

template <int Port,
          std::uint8_t Address,
          std::size_t Bytes,
          std::size_t Classes,
          typename Policy = IoStubPolicy>
struct SemanticWake {
    static_assert(Bytes > 0U && Classes > 0U, "semantic wake model dimensions must be non-zero");

    using Model = QuantDenseS8<Bytes, Classes>;

    [[nodiscard]] static Status run_once(
        const Model& model,
        const std::span<std::int8_t, Classes> logits,
        const std::int8_t threshold,
        const std::uint8_t center = 128U) noexcept
    {
        std::array<std::uint8_t, Bytes> raw{};
        auto read = I2c<Port, Policy>::read(Address, std::span<std::uint8_t>{raw});
        if (!read) {
            return read;
        }

        const auto input = center_u8(raw, center);
        if (!model.eval(std::span<const std::int8_t, Bytes>{input}, logits)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto best = logits[0];
        for (std::size_t i = 1U; i < Classes; ++i) {
            best = logits[i] > best ? logits[i] : best;
        }
        return best >= threshold ? status(Policy::wake_main()) : ok();
    }
};

}  // namespace arc::ulp::ml
