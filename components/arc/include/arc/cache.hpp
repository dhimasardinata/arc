#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_cache.h"
#include "esp_err.h"

#include "arc/sdk.hpp"

namespace arc {

struct Cache {
    static constexpr int invalidate = static_cast<int>(ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    static constexpr int unaligned = static_cast<int>(ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    static constexpr int dir_c2m = static_cast<int>(ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    static constexpr int dir_m2c = static_cast<int>(ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    static constexpr int type_data = static_cast<int>(ESP_CACHE_MSYNC_FLAG_TYPE_DATA);

    [[nodiscard]] static bool aligned(const void* const data) noexcept
    {
        return (reinterpret_cast<std::uintptr_t>(data) & (cache_line - 1U)) == 0U;
    }

    [[nodiscard]] static bool whole_lines(const std::size_t bytes) noexcept
    {
        return (bytes & (cache_line - 1U)) == 0U;
    }

    static esp_err_t sync(void* const data, const std::size_t bytes, const int flags) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_cache_msync(data, bytes, flags | type_data);
    }

    static esp_err_t strict(void* const data, const std::size_t bytes, const int flags) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        if (!aligned(data) || !whole_lines(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_cache_msync(data, bytes, (flags & ~unaligned) | type_data);
    }

    static esp_err_t to_device(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_c2m | unaligned);
    }

    static esp_err_t to_device_strict(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_c2m);
    }

    static esp_err_t from_device(void* const data, const std::size_t bytes) noexcept
    {
        return from_device_strict(data, bytes);
    }

    static esp_err_t from_device_strict(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_m2c);
    }

    [[deprecated("unaligned device invalidation can discard unrelated dirty data sharing a cache line; use arc::dmabuf/cachebuf and from_device_strict")]]
    static esp_err_t from_device_unaligned(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_m2c | unaligned);
    }

    static esp_err_t discard(void* const data, const std::size_t bytes) noexcept
    {
        return discard_strict(data, bytes);
    }

    static esp_err_t discard_strict(void* const data, const std::size_t bytes) noexcept
    {
        return strict(data, bytes, dir_c2m | invalidate);
    }

    [[deprecated("unaligned discard can invalidate unrelated dirty data sharing a cache line; use arc::dmabuf/cachebuf and discard_strict")]]
    static esp_err_t discard_unaligned(void* const data, const std::size_t bytes) noexcept
    {
        return sync(data, bytes, dir_c2m | invalidate | unaligned);
    }

    [[nodiscard]] static std::size_t line(const void* const data) noexcept
    {
        return esp_cache_get_line_size_by_addr(data);
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_device(const std::span<T, Extent> data) noexcept
    {
        return to_device(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t to_device_strict(const std::span<T, Extent> data) noexcept
    {
        return to_device_strict(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_device(const std::span<T, Extent> data) noexcept
    {
        return from_device(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t from_device_strict(const std::span<T, Extent> data) noexcept
    {
        return from_device_strict(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    [[deprecated("unaligned device invalidation can discard unrelated dirty data sharing a cache line; use arc::dmabuf/cachebuf and from_device_strict")]]
    static esp_err_t from_device_unaligned(const std::span<T, Extent> data) noexcept
    {
        return from_device_unaligned(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard(const std::span<T, Extent> data) noexcept
    {
        return discard(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    static esp_err_t discard_strict(const std::span<T, Extent> data) noexcept
    {
        return discard_strict(data.data(), data.size_bytes());
    }

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    [[deprecated("unaligned discard can invalidate unrelated dirty data sharing a cache line; use arc::dmabuf/cachebuf and discard_strict")]]
    static esp_err_t discard_unaligned(const std::span<T, Extent> data) noexcept
    {
        return discard_unaligned(data.data(), data.size_bytes());
    }
};

}  // namespace arc
