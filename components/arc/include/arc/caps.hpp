#pragma once

#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "esp_heap_caps.h"

namespace arc {

template <typename T>
struct CapsDel {
    void operator()(T* pointer) const noexcept
    {
        if (pointer == nullptr) {
            return;
        }

        std::destroy_at(pointer);
        heap_caps_free(pointer);
    }
};

template <typename T>
using CapsPtr = std::unique_ptr<T, CapsDel<T>>;

template <typename T, std::uint32_t Caps, typename... Args>
[[nodiscard]] inline CapsPtr<T> caps(Args&&... args) noexcept
{
    static_assert(std::is_nothrow_constructible_v<T, Args...>,
                  "capability-aware helpers assume nothrow construction");

    void* const storage = heap_caps_malloc(sizeof(T), Caps);
    if (storage == nullptr) {
        return {};
    }

    return CapsPtr<T>(new (storage) T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
[[nodiscard]] inline CapsPtr<T> inram(Args&&... args) noexcept
{
    return caps<T, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
[[nodiscard]] inline CapsPtr<T> psram(Args&&... args) noexcept
{
    return caps<T, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT>(std::forward<Args>(args)...);
}

}  // namespace arc
