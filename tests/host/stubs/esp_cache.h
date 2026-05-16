#pragma once

#include <cstddef>

#include "esp_err.h"

#define ESP_CACHE_MSYNC_FLAG_INVALIDATE 0x01
#define ESP_CACHE_MSYNC_FLAG_UNALIGNED 0x02
#define ESP_CACHE_MSYNC_FLAG_DIR_C2M 0x04
#define ESP_CACHE_MSYNC_FLAG_DIR_M2C 0x08
#define ESP_CACHE_MSYNC_FLAG_TYPE_DATA 0x10

inline std::size_t esp_cache_last_msync_bytes{};
inline int esp_cache_last_msync_flags{};

inline esp_err_t esp_cache_msync(void* const data, const std::size_t bytes, const int flags)
{
    esp_cache_last_msync_bytes = bytes;
    esp_cache_last_msync_flags = flags;
    return data != nullptr && bytes != 0U ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline std::size_t esp_cache_get_line_size_by_addr(const void*)
{
    return 64U;
}
