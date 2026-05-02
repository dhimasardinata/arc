#pragma once

#include <cstdint>

#include "esp_timer.h"
#include "soc/soc_caps.h"

namespace arc {

struct Time {
    static_assert(SOC_SYSTIMER_SUPPORTED, "system timer is not supported on this target");

    using Tick = std::uint64_t;

    // esp_timer_get_time() is wall-clock safe but not a hot-loop primitive; use Clock::tick()
    // for cycle-budgeted Core 1 paths.
    [[nodiscard]] static Tick us() noexcept
    {
        return static_cast<Tick>(esp_timer_get_time());
    }

    [[nodiscard]] static Tick ms() noexcept
    {
        return us() / 1000ULL;
    }

    [[nodiscard]] static Tick next() noexcept
    {
        const auto value = esp_timer_get_next_alarm();
        return value < 0 ? 0ULL : static_cast<Tick>(value);
    }

    [[nodiscard]] static Tick wake() noexcept
    {
        const auto value = esp_timer_get_next_alarm_for_wake_up();
        return value < 0 ? 0ULL : static_cast<Tick>(value);
    }

    [[nodiscard]] static Tick since(const Tick start) noexcept
    {
        return us() - start;
    }

    [[nodiscard]] static bool due(const Tick start, const Tick span) noexcept
    {
        return since(start) >= span;
    }
};

}  // namespace arc
