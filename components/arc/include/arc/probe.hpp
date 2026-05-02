#pragma once

#include <cstdint>
#include <limits>

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

struct JitterStats {
    std::uint32_t samples{};
    std::int32_t min{std::numeric_limits<std::int32_t>::max()};
    std::int32_t max{std::numeric_limits<std::int32_t>::min()};
    std::uint64_t abs_total{};

    IRAM_ATTR [[gnu::always_inline]] inline void add(const std::int32_t cycles) noexcept
    {
        min = cycles < min ? cycles : min;
        max = cycles > max ? cycles : max;
        abs_total += abs(cycles);
        samples += 1U;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::uint32_t avg_abs() const noexcept
    {
        return samples == 0U ? 0U : static_cast<std::uint32_t>(abs_total / samples);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::uint32_t max_abs() const noexcept
    {
        const auto lo = abs(min);
        const auto hi = abs(max);
        return lo > hi ? lo : hi;
    }

    IRAM_ATTR [[gnu::always_inline]] inline void clear() noexcept
    {
        samples = 0U;
        min = std::numeric_limits<std::int32_t>::max();
        max = std::numeric_limits<std::int32_t>::min();
        abs_total = 0U;
    }

private:
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static std::uint32_t abs(const std::int32_t value) noexcept
    {
        if (value >= 0) {
            return static_cast<std::uint32_t>(value);
        }
        return static_cast<std::uint32_t>(-(value + 1)) + 1U;
    }
};

}  // namespace arc
