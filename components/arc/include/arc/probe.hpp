#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "esp_cpu.h"

#include "arc/clock.hpp"

namespace arc {

struct Probe {
    esp_cpu_cycle_count_t start{};

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline Probe now() noexcept
    {
        return Probe{Clock::tick()};
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline esp_cpu_cycle_count_t lap() const noexcept
    {
        return static_cast<esp_cpu_cycle_count_t>(Clock::tick() - start);
    }
};

struct CycleStats {
    std::uint32_t samples{};
    esp_cpu_cycle_count_t min{UINT32_MAX};
    esp_cpu_cycle_count_t max{};
    std::uint64_t total{};

    IRAM_ATTR [[gnu::always_inline]] inline void add(const esp_cpu_cycle_count_t cycles) noexcept
    {
        min = cycles < min ? cycles : min;
        max = cycles > max ? cycles : max;
        total += cycles;
        samples += 1U;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline esp_cpu_cycle_count_t avg() const noexcept
    {
        return samples == 0U ? 0U : static_cast<esp_cpu_cycle_count_t>(total / samples);
    }

    IRAM_ATTR [[gnu::always_inline]] inline void clear() noexcept
    {
        samples = 0U;
        min = UINT32_MAX;
        max = 0U;
        total = 0U;
    }
};

}  // namespace arc
