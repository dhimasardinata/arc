#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

#define MALLOC_CAP_SPIRAM 0x01U
#define MALLOC_CAP_8BIT 0x02U
#define MALLOC_CAP_INTERNAL 0x04U
#define MALLOC_CAP_DMA 0x08U
#define MALLOC_CAP_SIMD 0x10U
#define MALLOC_CAP_CACHE_ALIGNED 0x20U
#define MALLOC_CAP_RTCRAM 0x40U
#define MALLOC_CAP_DMA_DESC_AHB 0x80U
#define MALLOC_CAP_DMA_DESC_AXI 0x100U

inline std::size_t heap_caps_last_calloc_bytes{};

inline std::size_t heap_caps_get_free_size(std::uint32_t) noexcept
{
    return 0U;
}

inline void* heap_caps_malloc(const std::size_t size, std::uint32_t) noexcept
{
    return std::malloc(size);
}

inline void* heap_caps_aligned_alloc(
    std::size_t alignment,
    const std::size_t size,
    std::uint32_t) noexcept
{
    if (size == 0U || alignment == 0U) {
        return nullptr;
    }
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }
    if ((alignment & (alignment - 1U)) != 0U) {
        return nullptr;
    }
    if (size > std::numeric_limits<std::size_t>::max() - (alignment - 1U)) {
        return nullptr;
    }

    const auto rounded = (size + alignment - 1U) & ~(alignment - 1U);
    void* out{};
    return posix_memalign(&out, alignment, rounded) == 0 ? out : nullptr;
}

inline void* heap_caps_aligned_calloc(
    const std::size_t alignment,
    const std::size_t count,
    const std::size_t size,
    const std::uint32_t caps) noexcept
{
    if (count != 0U && size > std::numeric_limits<std::size_t>::max() / count) {
        return nullptr;
    }
    const auto bytes = count * size;
    heap_caps_last_calloc_bytes = bytes;
    void* const out = heap_caps_aligned_alloc(alignment, bytes, caps);
    if (out != nullptr) {
        std::memset(out, 0, bytes);
    }
    return out;
}

inline void heap_caps_free(void* const pointer) noexcept
{
    std::free(pointer);
}
