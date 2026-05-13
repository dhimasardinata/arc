#pragma once

#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc {

struct HilDeadline {
    std::uint32_t expected_tick{};
    std::uint32_t observed_tick{};
    std::uint32_t tolerance_ticks{};
};

struct Hil {
    [[nodiscard]] static constexpr bool within(const HilDeadline deadline) noexcept
    {
        const auto diff = deadline.observed_tick > deadline.expected_tick
            ? deadline.observed_tick - deadline.expected_tick
            : deadline.expected_tick - deadline.observed_tick;
        return diff <= deadline.tolerance_ticks;
    }
};

enum class HilRole : std::uint8_t {
    dut,
    physical_chaos,
    radio_jammer,
};

enum class HilFault : std::uint8_t {
    i2c_short,
    cs_drop,
    can_spam,
    espnow_jam,
};

struct HilAction {
    HilRole role{};
    HilFault fault{};
    std::uint32_t at_tick{};
    std::uint32_t duration_ticks{};
};

template <std::size_t Actions>
struct HilScript {
    static_assert(Actions > 0U, "HIL script needs at least one action");

    HilAction actions[Actions]{};

    template <typename Policy>
    [[nodiscard]] Status run_due(const std::uint32_t tick) const noexcept
    {
        for (const auto action : actions) {
            if (action.duration_ticks == 0U || tick < action.at_tick || tick >= action.at_tick + action.duration_ticks) {
                continue;
            }
            const auto applied = Policy::apply(action);
            if (applied != ESP_OK) {
                return Status{fail(applied)};
            }
        }
        return ok();
    }
};

}  // namespace arc
