#pragma once

#include <cstddef>
#include <span>
#include <type_traits>

#include "esp_attr.h"

namespace arc {

IRAM_ATTR [[gnu::always_inline]] inline void prefetch(const void* const ptr) noexcept
{
    __builtin_prefetch(ptr, 0, 3);
}

IRAM_ATTR [[gnu::always_inline]] inline void prefetch_write(const void* const ptr) noexcept
{
    __builtin_prefetch(ptr, 1, 3);
}

template <typename T, std::size_t Extent>
    requires std::is_trivially_copyable_v<T>
IRAM_ATTR [[gnu::always_inline]] inline void prefetch(const std::span<T, Extent> data) noexcept
{
    if (data.data() != nullptr) {
        prefetch(data.data());
    }
}

template <typename T, std::size_t Extent>
    requires std::is_trivially_copyable_v<T>
IRAM_ATTR [[gnu::always_inline]] inline void prefetch_write(const std::span<T, Extent> data) noexcept
{
    if (data.data() != nullptr) {
        prefetch_write(data.data());
    }
}

}  // namespace arc
