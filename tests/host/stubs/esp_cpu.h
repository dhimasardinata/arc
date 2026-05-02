#pragma once

#include <cstdint>

using esp_cpu_cycle_count_t = std::uint32_t;

inline esp_cpu_cycle_count_t esp_cpu_get_cycle_count() noexcept
{
    static esp_cpu_cycle_count_t tick{};
    return ++tick;
}

inline int esp_cpu_get_core_id() noexcept
{
    return 0;
}
