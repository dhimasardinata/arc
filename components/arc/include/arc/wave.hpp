#pragma once

#include <cstdint>

#include "esp_attr.h"

#include "arc/clock.hpp"
#include "arc/fence.hpp"

namespace arc {

template <typename Pin, std::uint32_t HalfPeriodMicroseconds, std::uint32_t CpuFrequencyMHz>
struct Wave {
    static void setup()
    {
        Pin::out();
        Pin::template write<false>();
    }

    IRAM_ATTR static void run() noexcept
    {
        const auto half_period_cycles =
            static_cast<esp_cpu_cycle_count_t>(imm<HalfPeriodMicroseconds>()) *
            static_cast<esp_cpu_cycle_count_t>(imm<CpuFrequencyMHz>());

        while (true) {
            Pin::template write<true>();
            fence();
            Clock::spin(half_period_cycles);
            Pin::template write<false>();
            fence();
            Clock::spin(half_period_cycles);
        }
    }

private:
    template <std::uint32_t Value>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t imm() noexcept
    {
        volatile std::uint32_t value = (Value >> 24U) & 0xFFU;
        value = (value << 8U) | ((Value >> 16U) & 0xFFU);
        value = (value << 8U) | ((Value >> 8U) & 0xFFU);
        value = (value << 8U) | (Value & 0xFFU);
        return value;
    }
};

}  // namespace arc
