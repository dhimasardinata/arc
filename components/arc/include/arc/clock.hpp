#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "esp_cpu.h"

#include "arc/fence.hpp"

namespace arc {

struct Clock {
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline esp_cpu_cycle_count_t tick() noexcept
    {
        return esp_cpu_get_cycle_count();
    }

    [[nodiscard]] static constexpr esp_cpu_cycle_count_t us(
        const std::uint32_t time_us,
        const std::uint32_t mhz) noexcept
    {
        return static_cast<esp_cpu_cycle_count_t>(time_us * mhz);
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
};

}  // namespace arc
