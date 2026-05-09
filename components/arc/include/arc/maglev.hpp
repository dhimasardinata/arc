#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "arc/dsp.hpp"
#include "arc/result.hpp"

namespace arc {

template <std::size_t Axes>
struct MagLev {
    static_assert(Axes > 0U, "MagLev needs at least one controlled axis");

    using Plant = dsp::StateSpace<float, Axes * 2U, Axes, Axes>;
    using State = typename Plant::State;
    using Model = typename Plant::Model;
    using Vec = std::array<float, Axes>;

    struct Limits {
        float min{-1.0F};
        float max{1.0F};
    };

    [[nodiscard]] static Result<Vec> stabilize(
        State& state,
        const Model& model,
        const Vec& hall_error,
        const Limits limits = {}) noexcept
    {
        auto duty = Plant::step(state, model, hall_error);
        for (auto& value : duty) {
            value = clamp(value, limits);
        }
        return duty;
    }

    template <typename Policy>
    [[nodiscard]] static Status tick(
        State& state,
        const Model& model,
        const Limits limits = {}) noexcept
    {
        auto hall = Policy::sample_hall();
        if (!hall) {
            return Status{fail(hall.error())};
        }
        auto duty = stabilize(state, model, *hall, limits);
        if (!duty) {
            return Status{fail(duty.error())};
        }
        return status(Policy::drive_bridge(std::span<const float, Axes>{*duty}));
    }

private:
    [[nodiscard]] static constexpr float clamp(const float value, const Limits limits) noexcept
    {
        return value < limits.min ? limits.min : (value > limits.max ? limits.max : value);
    }
};

}  // namespace arc
