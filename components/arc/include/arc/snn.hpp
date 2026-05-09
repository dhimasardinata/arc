#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::ml {

struct LifParams {
    std::int16_t threshold{1024};
    std::int16_t leak{1};
    std::int16_t reset{};
};

template <std::size_t Inputs, std::size_t Neurons>
struct Snn {
    static_assert(Inputs > 0U && Neurons > 0U, "SNN dimensions must be non-zero");

    std::span<const std::int8_t, Inputs * Neurons> weights{};
    std::array<std::int16_t, Neurons> membrane{};
    LifParams params{};

    [[nodiscard]] Status step(
        const std::span<const std::int8_t, Inputs> spikes,
        const std::span<std::uint8_t, Neurons> out_spikes) noexcept
    {
        if (weights.size() != Inputs * Neurons) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t neuron = 0U; neuron < Neurons; ++neuron) {
            auto acc = static_cast<std::int32_t>(membrane[neuron]);
            const auto base = neuron * Inputs;
            std::size_t input = 0U;
            for (; input + simd::int8x16_lanes <= Inputs; input += simd::int8x16_lanes) {
                acc = simd::mac_s8x16(
                    acc,
                    simd::load_s8x16(spikes.data() + input),
                    simd::load_s8x16(weights.data() + base + input));
            }
            for (; input < Inputs; ++input) {
                acc += static_cast<std::int32_t>(spikes[input]) * weights[base + input];
            }
            acc -= params.leak;
            if (acc >= params.threshold) {
                out_spikes[neuron] = 1U;
                membrane[neuron] = params.reset;
            } else {
                out_spikes[neuron] = 0U;
                membrane[neuron] = saturate(acc);
            }
        }
        return ok();
    }

private:
    [[nodiscard]] static constexpr std::int16_t saturate(const std::int32_t value) noexcept
    {
        return value > 32767 ? 32767 : (value < -32768 ? -32768 : static_cast<std::int16_t>(value));
    }
};

}  // namespace arc::ml
