#pragma once

#include <cstddef>

#include "sdkconfig.h"

namespace arc {

#if defined(CONFIG_CACHE_L1_CACHE_LINE_SIZE)
inline constexpr std::size_t cache_line = CONFIG_CACHE_L1_CACHE_LINE_SIZE;
#elif defined(CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE)
inline constexpr std::size_t cache_line = CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE;
#else
inline constexpr std::size_t cache_line = 64U;
#endif

static_assert((cache_line & (cache_line - 1U)) == 0U, "cache line size must be a power of two");

}  // namespace arc
