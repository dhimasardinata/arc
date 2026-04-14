#pragma once

#include <cstdint>

#include "sdkconfig.h"

namespace arc::cfg {

inline constexpr int led = CONFIG_ARC_LED;
inline constexpr int sense = CONFIG_ARC_SENSE;
inline constexpr std::uint32_t stack = CONFIG_ARC_STACK;
inline constexpr std::uint32_t half_us = CONFIG_ARC_HALF_US;
inline constexpr std::uint32_t mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

}  // namespace arc::cfg
