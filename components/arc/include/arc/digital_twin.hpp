#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

#include "arc/dsp.hpp"
#include "arc/result.hpp"

namespace arc::hil {

template <typename T, std::size_t Outputs>
struct DigitalTwinSample {
    std::array<T, Outputs> output{};
    std::uint32_t captured_ticks{};
};

template <typename T, std::size_t Inputs, std::size_t Outputs>
struct DigitalTwinTrajectory {
    std::array<T, Inputs> input{};
    std::array<T, Outputs> output{};
    std::uint32_t candidate{};
    std::uint32_t step{};
};

template <typename T, std::size_t States, std::size_t Inputs, std::size_t Outputs>
struct DigitalTwin {
    using Plant = dsp::StateSpace<T, States, Inputs, Outputs>;
    using State = typename Plant::State;
    using Model = typename Plant::Model;
    using InputVec = typename Plant::InputVec;
    using OutputVec = typename Plant::OutputVec;
    using Sample = DigitalTwinSample<T, Outputs>;
    using Trajectory = DigitalTwinTrajectory<T, Inputs, Outputs>;

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

    template <std::size_t Horizon, std::size_t Candidates, typename ScorePolicy>
        requires std::convertible_to<std::invoke_result_t<ScorePolicy, std::size_t, std::size_t, const OutputVec&>, T>
    [[nodiscard]] static Result<std::size_t> choose(
        State state,
        const Model& model,
        const std::span<const InputVec, Horizon * Candidates> inputs,
        ScorePolicy&& score) noexcept(noexcept(std::invoke(std::declval<ScorePolicy&>(),
                                                           std::size_t{},
                                                           std::size_t{},
                                                           std::declval<const OutputVec&>())))
    {
        static_assert(Horizon > 0U,
                      "[ARC ERROR] arc::hil::DigitalTwin choose needs a forecast horizon. Action: set Horizon above zero.");
        static_assert(Candidates > 0U,
                      "[ARC ERROR] arc::hil::DigitalTwin choose needs candidates. Action: set Candidates above zero.");
        if (!valid(inputs)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t best{};
        T best_cost{};
        bool found = false;
        for (std::size_t candidate = 0U; candidate < Candidates; ++candidate) {
            auto copy = state;
            T cost{};
            for (std::size_t step = 0U; step < Horizon; ++step) {
                const auto output = Plant::step(copy, model, inputs[candidate * Horizon + step]);
                cost += static_cast<T>(std::invoke(score, candidate, step, output));
            }
            if (!found || cost < best_cost) {
                best = candidate;
                best_cost = cost;
                found = true;
            }
        }
        return best;
    }

    template <std::size_t Horizon, std::size_t Candidates, typename ScorePolicy, typename Lane>
        requires std::convertible_to<std::invoke_result_t<ScorePolicy, std::size_t, std::size_t, const OutputVec&>, T> &&
        requires(Lane& lane, std::span<const Trajectory, Horizon> points) {
            { lane.push(points) } -> std::convertible_to<std::size_t>;
        }
    [[nodiscard]] static Result<std::size_t> publish(
        State state,
        const Model& model,
        const std::span<const InputVec, Horizon * Candidates> inputs,
        const std::span<Trajectory, Horizon> scratch,
        Lane& lane,
        ScorePolicy&& score) noexcept(noexcept(std::invoke(std::declval<ScorePolicy&>(),
                                                           std::size_t{},
                                                           std::size_t{},
                                                           std::declval<const OutputVec&>())))
    {
        static_assert(Horizon > 0U,
                      "[ARC ERROR] arc::hil::DigitalTwin publish needs a forecast horizon. Action: set Horizon above zero.");
        static_assert(Candidates > 0U,
                      "[ARC ERROR] arc::hil::DigitalTwin publish needs candidates. Action: set Candidates above zero.");
        if (!valid(inputs) || !valid(scratch)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto best = choose<Horizon, Candidates>(state, model, inputs, score);
        if (!best) {
            return fail(best.error());
        }

        auto replay = state;
        const auto base = *best * Horizon;
        for (std::size_t step = 0U; step < Horizon; ++step) {
            const auto input = inputs[base + step];
            scratch[step] = Trajectory{
                .input = input,
                .output = Plant::step(replay, model, input),
                .candidate = static_cast<std::uint32_t>(*best),
                .step = static_cast<std::uint32_t>(step),
            };
        }

        const auto pushed = lane.push(std::span<const Trajectory, Horizon>{scratch.data(), Horizon});
        return pushed == Horizon ? Result<std::size_t>{pushed} : fail(ESP_ERR_NO_MEM);
    }

private:
    template <typename TSpan, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid(const std::span<TSpan, Extent> span) noexcept
    {
        return span.empty() || span.data() != nullptr;
    }
};

}  // namespace arc::hil
