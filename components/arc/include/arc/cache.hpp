#pragma once

#include <cstddef>
#include <span>
#include <type_traits>

#include "esp_cache.h"
#include "esp_err.h"

namespace arc {

struct Cache {
    static esp_err_t sync(void* const data, const std::size_t bytes, const int flags) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_cache_msync(data, bytes, flags | ESP_CACHE_MSYNC_FLAG_TYPE_DATA);
    }

    static esp_err_t to_device(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }

    static esp_err_t from_device(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }

    static esp_err_t discard(void* const data, const std::size_t bytes) noexcept
    {
        return sync(
            data,
            bytes,
            ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }

    [[nodiscard]] static std::size_t line(const void* const data) noexcept
    {
        return esp_cache_get_line_size_by_addr(data);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_device(const std::span<T> data) noexcept
    {
        return to_device(data.data(), data.size_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_device(const std::span<T> data) noexcept
    {
        return from_device(data.data(), data.size_bytes());
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard(const std::span<T> data) noexcept
    {
        return discard(data.data(), data.size_bytes());
    }
};

}  // namespace arc
