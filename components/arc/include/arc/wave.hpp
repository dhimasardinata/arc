#pragma once

#include <cstdint>

#include "esp_attr.h"

#include "arc/clock.hpp"
#include "arc/fence.hpp"

namespace arc {

template <typename Pin,
          std::uint32_t HalfPeriodMicroseconds,
          std::uint32_t CpuFrequencyMHz,
          ClockSource Source = ClockSource::systimer>
struct Wave {
    static_assert(
        Source != ClockSource::cycle_counter || !clock_dfs_possible(),
        "arc::Wave cycle-counter timing is not DFS-safe when CONFIG_PM_ENABLE is set; use the default systimer source or hold arc::CpuLock and request ClockSource::locked_cycle_counter");

    static void setup()
    {
        Pin::out();
        Pin::template write<false>();
    }

    IRAM_ATTR static void run() noexcept
    {
        if constexpr (Source == ClockSource::systimer) {
            run_systimer();
        } else {
            run_cycles();
        }
    }

private:
    static void run_systimer() noexcept
    {
        const auto half_period_us = imm<HalfPeriodMicroseconds>();

        while (true) {
            Pin::template write<true>();
            fence();
            Clock::spin_us(half_period_us);
            Pin::template write<false>();
            fence();
            Clock::spin_us(half_period_us);
        }
    }

    IRAM_ATTR static void run_cycles() noexcept
    {
        const auto half_period_cycles =
            Clock::cycles(imm<HalfPeriodMicroseconds>(), imm<CpuFrequencyMHz>());

        while (true) {
            Pin::template write<true>();
            fence();
            Clock::spin(half_period_cycles);
            Pin::template write<false>();
            fence();
            Clock::spin(half_period_cycles);
        }
    }

    template <std::uint32_t Value>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t imm() noexcept
    {
        std::uint32_t value = Value;
        __asm__ __volatile__("" : "+r"(value));
        return value;
    }
};

}  // namespace arc
