#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "esp_cpu.h"
#if __has_include("esp_timer.h")
#include "esp_timer.h"
#endif
#include "sdkconfig.h"

#include "arc/fence.hpp"

namespace arc {

enum class ClockSource : std::uint8_t {
    systimer,
    cycle_counter,
    locked_cycle_counter,
};

[[nodiscard]] consteval bool clock_dfs_possible() noexcept
{
#if defined(CONFIG_PM_ENABLE) && CONFIG_PM_ENABLE
    return true;
#else
    return false;
#endif
}

struct Clock {
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline esp_cpu_cycle_count_t tick() noexcept
    {
        return esp_cpu_get_cycle_count();
    }

    [[nodiscard]] static constexpr esp_cpu_cycle_count_t cycles(
        const std::uint32_t time_us,
        const std::uint32_t mhz) noexcept
    {
        return static_cast<esp_cpu_cycle_count_t>(time_us * mhz);
    }

    [[nodiscard]] static constexpr esp_cpu_cycle_count_t us(
        const std::uint32_t time_us,
        const std::uint32_t mhz) noexcept
    {
        return cycles(time_us, mhz);
    }

    [[nodiscard]] static constexpr std::uint32_t default_mhz() noexcept
    {
#if defined(CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ)
        return CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
#else
        return 240U;
#endif
    }

    [[nodiscard]] static inline std::uint64_t wall_us() noexcept
    {
#if __has_include("esp_timer.h")
        return static_cast<std::uint64_t>(esp_timer_get_time());
#else
        return static_cast<std::uint64_t>(tick() / default_mhz());
#endif
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void spin(
        const esp_cpu_cycle_count_t cycles) noexcept
    {
        const auto start = tick();
        fence();
        while (static_cast<esp_cpu_cycle_count_t>(tick() - start) < cycles) {
            pause();
        }
        fence();
    }

    static inline void spin_us(const std::uint32_t time_us) noexcept
    {
        const auto start = wall_us();
        fence();
        while ((wall_us() - start) < time_us) {
            pause();
        }
        fence();
    }

    template <ClockSource Source>
    static consteval void require_stable_source() noexcept
    {
        static_assert(
            Source != ClockSource::cycle_counter || !clock_dfs_possible(),
            "ClockSource::cycle_counter is not DFS-safe when CONFIG_PM_ENABLE is set; use ClockSource::systimer or hold arc::CpuLock and request ClockSource::locked_cycle_counter");
    }
};

}  // namespace arc
