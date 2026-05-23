#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::matrix {

enum class RouteDir : std::uint8_t {
    input,
    output,
};

struct Route {
    std::uint32_t signal{};
    std::uint32_t gpio{};
    RouteDir dir{RouteDir::output};
    bool invert{};
    bool open_drain{};
};

struct FaultSample {
    std::uint32_t expected_hz{};
    std::uint32_t measured_hz{};
    std::uint32_t tolerance_hz{};
};

template <std::size_t Spares>
struct FlexRoutePlan {
    static_assert(Spares > 0U, "flex route needs at least one spare pin");

    Route active{};
    std::array<std::uint32_t, Spares> spare_gpio{};
};

struct FlexRoute {
    [[nodiscard]] static constexpr bool failed(const FaultSample sample) noexcept
    {
        if (sample.expected_hz == 0U) {
            return false;
        }
        const auto lo = sample.expected_hz > sample.tolerance_hz ? sample.expected_hz - sample.tolerance_hz : 0U;
        const auto room = UINT32_MAX - sample.expected_hz;
        const auto hi = sample.tolerance_hz > room ? UINT32_MAX : sample.expected_hz + sample.tolerance_hz;
        return sample.measured_hz < lo || sample.measured_hz > hi;
    }

    template <typename Policy>
    [[nodiscard]] static Status apply(const Route route) noexcept
    {
        if (route.gpio == 0xFFFF'FFFFU) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (route.dir == RouteDir::output) {
            return status(Policy::matrix_out(route.gpio, route.signal, route.invert, route.open_drain));
        }
        return status(Policy::matrix_in(route.gpio, route.signal, route.invert));
    }

    template <typename Policy, std::size_t Spares>
    [[nodiscard]] static Result<Route> heal(
        FlexRoutePlan<Spares>& plan,
        const FaultSample sample) noexcept
    {
        if (!failed(sample)) {
            return plan.active;
        }

        for (auto& spare : plan.spare_gpio) {
            if (spare == 0xFFFF'FFFFU) {
                continue;
            }
            const auto next = Route{
                .signal = plan.active.signal,
                .gpio = spare,
                .dir = plan.active.dir,
                .invert = plan.active.invert,
                .open_drain = plan.active.open_drain,
            };
            if (auto detached = detach<Policy>(plan.active); !detached) {
                return fail(detached.error());
            }
            if (auto applied = apply<Policy>(next); !applied) {
                return fail(applied.error());
            }
            spare = 0xFFFF'FFFFU;
            plan.active = next;
            return next;
        }
        return fail(ESP_ERR_NOT_FOUND);
    }

    template <typename Policy>
    [[nodiscard]] static Status detach(const Route route) noexcept
    {
        if constexpr (requires { Policy::matrix_detach(route.gpio, route.dir); }) {
            return status(Policy::matrix_detach(route.gpio, route.dir));
        } else {
            return ok();
        }
    }
};

}  // namespace arc::matrix
