#pragma once

#include <cstdint>

#include "sdkconfig.h"

namespace app::cfg {

inline constexpr int led = CONFIG_ARC_UDP_LED;
inline constexpr std::uint32_t stack = CONFIG_ARC_UDP_STACK;
inline constexpr std::uint32_t net_stack = CONFIG_ARC_UDP_NET_STACK;
inline constexpr std::uint32_t half_us = CONFIG_ARC_UDP_HALF_US;
inline constexpr std::uint32_t fast_half_us = CONFIG_ARC_UDP_FAST_HALF_US;
inline constexpr std::uint32_t cmd_ms = CONFIG_ARC_UDP_CMD_MS;
inline constexpr std::uint32_t port = CONFIG_ARC_UDP_PORT;
inline constexpr std::uint32_t mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

inline constexpr char ssid[] = CONFIG_ARC_UDP_SSID;
inline constexpr char pass[] = CONFIG_ARC_UDP_PASS;
inline constexpr char host[] = CONFIG_ARC_UDP_HOST;

}  // namespace app::cfg
