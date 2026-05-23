#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dsp.hpp"
#include "arc/result.hpp"

namespace arc::hil {

template <typename T, std::size_t Outputs>
struct DigitalTwinSample {
    std::array<T, Outputs> output{};
    std::uint32_t captured_ticks{};
};

template <typename T, std::size_t States, std::size_t Inputs, std::size_t Outputs>
struct DigitalTwin {
    using Plant = dsp::StateSpace<T, States, Inputs, Outputs>;
    using State = typename Plant::State;
    using Model = typename Plant::Model;
    using InputVec = typename Plant::InputVec;
    using OutputVec = typename Plant::OutputVec;
    using Sample = DigitalTwinSample<T, Outputs>;

    template <typename CapturePolicy, typename EncoderPolicy>
    [[nodiscard]] static Result<Sample> tick(
        State& state,
        const Model& model,
        const InputVec input) noexcept
    {
        auto captured = CapturePolicy::sample_ticks();
        if (!captured) {
            return fail(captured.error());
        }
        const auto output = Plant::step(state, model, input);
        if (const auto err = EncoderPolicy::emit(std::span<const T, Outputs>{output}); err != ESP_OK) {
            return fail(err);
        }
        return Sample{.output = output, .captured_ticks = *captured};
    }

    template <std::size_t Horizon>
    [[nodiscard]] static Result<std::size_t> forecast(
        State state,
        const Model& model,
        const std::span<const InputVec, Horizon> inputs,
        const std::span<OutputVec, Horizon> outputs) noexcept
    {
        static_assert(Horizon > 0U,
                      "[ARC ERROR] arc::hil::DigitalTwin forecast needs steps. Action: set Horizon above zero.");
        if (!valid(inputs) || !valid(outputs)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        for (std::size_t i = 0U; i < Horizon; ++i) {
            outputs[i] = Plant::step(state, model, inputs[i]);
        }
        return Horizon;
    }

private:
    template <typename TSpan, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid(const std::span<TSpan, Extent> span) noexcept
    {
        return span.empty() || span.data() != nullptr;
    }
};

}  // namespace arc::hil
