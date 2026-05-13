#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc {

template <std::size_t Axes>
struct MotionStep {
    std::uint32_t mask{};
    std::uint32_t ticks{1U};
};

template <std::size_t Axes>
struct MotionSegment {
    std::array<std::int32_t, Axes> delta{};
    std::uint32_t ticks_step{1U};
};

template <std::size_t Axes>
struct MotionPlan {
    static_assert(Axes > 0U && Axes <= 32U, "Motion axes must fit a 32-bit step mask");

    [[nodiscard]] static Result<std::span<const MotionStep<Axes>>> line(
        std::span<MotionStep<Axes>> out,
        const MotionSegment<Axes>& segment) noexcept
    {
        std::array<std::uint32_t, Axes> distance{};
        std::uint32_t major{};
        for (std::size_t axis = 0; axis < Axes; ++axis) {
            const auto value = segment.delta[axis];
            distance[axis] = static_cast<std::uint32_t>(value < 0 ? -value : value);
            major = distance[axis] > major ? distance[axis] : major;
        }

        if (major == 0U) {
            return out.first(0);
        }
        if (out.size() < major) {
            return fail(ESP_ERR_NO_MEM);
        }

        std::array<std::uint32_t, Axes> error{};
        for (std::size_t step = 0; step < major; ++step) {
            std::uint32_t mask{};
            for (std::size_t axis = 0; axis < Axes; ++axis) {
                error[axis] += distance[axis];
                if (error[axis] >= major) {
                    error[axis] -= major;
                    mask |= std::uint32_t{1U} << axis;
                }
            }
            out[step] = MotionStep<Axes>{.mask = mask, .ticks = segment.ticks_step};
        }
        return out.first(major);
    }
};

template <typename T>
struct SCurvePoint {
    T position{};
    T velocity{};
    T acceleration{};
};

template <typename T>
struct SCurve {
    [[nodiscard]] static constexpr SCurvePoint<T> sample(
        const T distance,
        const T duration,
        const T t) noexcept
    {
        if (duration <= T{}) {
            return {};
        }

        const auto u_raw = t <= T{} ? T{} : (t >= duration ? T{1} : t / duration);
        const auto u2 = u_raw * u_raw;
        const auto u3 = u2 * u_raw;
        const auto u4 = u3 * u_raw;
        const auto u5 = u4 * u_raw;
        const auto s = (T{10} * u3) - (T{15} * u4) + (T{6} * u5);
        const auto ds = ((T{30} * u2) - (T{60} * u3) + (T{30} * u4)) / duration;
        const auto dds = ((T{60} * u_raw) - (T{180} * u2) + (T{120} * u3)) / (duration * duration);
        return {
            .position = distance * s,
            .velocity = distance * ds,
            .acceleration = distance * dds,
        };
    }
};

}  // namespace arc
