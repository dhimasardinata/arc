#pragma once

#include <bit>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_heap_caps.h"

#include "arc/sdk.hpp"

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

namespace detail {

// Cache maintenance works in whole lines, so cache/DMA-facing buffers get
// hidden physical padding while keeping their public span size unchanged.
template <std::uint32_t Caps>
inline constexpr bool cache_caps =
    (Caps &
     (MALLOC_CAP_DMA |
      MALLOC_CAP_CACHE_ALIGNED |
      MALLOC_CAP_SIMD |
      MALLOC_CAP_DMA_DESC_AHB |
      MALLOC_CAP_DMA_DESC_AXI)) != 0U;

[[nodiscard]] constexpr bool can_round(
    const std::size_t value,
    const std::size_t align) noexcept
{
    return value <= (std::numeric_limits<std::size_t>::max() - (align - 1U));
}

[[nodiscard]] constexpr std::size_t round_up(
    const std::size_t value,
    const std::size_t align) noexcept
{
    return (value + align - 1U) & ~(align - 1U);
}

template <std::uint32_t Caps>
[[nodiscard]] constexpr std::size_t allocation_bytes(
    const std::size_t bytes,
    const std::size_t align) noexcept
{
    if constexpr (cache_caps<Caps>) {
        return round_up(bytes, align);
    } else {
        static_cast<void>(align);
        return bytes;
    }
}

}  // namespace detail

template <typename T>
class CapsBuf {
public:
    constexpr CapsBuf() noexcept = default;

    constexpr CapsBuf(
        T* const data,
        const std::size_t size,
        const std::size_t storage_bytes) noexcept
        : data_(data)
        , size_(size)
        , storage_bytes_(storage_bytes)
    {
    }

    CapsBuf(const CapsBuf&) = delete;
    CapsBuf& operator=(const CapsBuf&) = delete;

    constexpr CapsBuf(CapsBuf&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
        , storage_bytes_(other.storage_bytes_)
    {
        other.data_ = nullptr;
        other.size_ = 0U;
        other.storage_bytes_ = 0U;
    }

    constexpr CapsBuf& operator=(CapsBuf&& other) noexcept
    {
        if (this != &other) {
            reset();
            data_ = other.data_;
            size_ = other.size_;
            storage_bytes_ = other.storage_bytes_;
            other.data_ = nullptr;
            other.size_ = 0U;
            other.storage_bytes_ = 0U;
        }
        return *this;
    }

    ~CapsBuf()
    {
        reset();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return data_ != nullptr;
    }

    [[nodiscard]] constexpr T* data() noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr const T* data() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return size_;
    }

    [[nodiscard]] constexpr std::size_t bytes() const noexcept
    {
        return size_ * sizeof(T);
    }

    [[nodiscard]] constexpr std::size_t storage_bytes() const noexcept
    {
        return storage_bytes_;
    }

    [[nodiscard]] constexpr T& operator[](const std::size_t index) noexcept
    {
        return data_[index];
    }

    [[nodiscard]] constexpr const T& operator[](const std::size_t index) const noexcept
    {
        return data_[index];
    }

    [[nodiscard]] constexpr std::span<T> view() noexcept
    {
        return {data_, size_};
    }

    [[nodiscard]] constexpr std::span<const T> view() const noexcept
    {
        return {data_, size_};
    }

    [[nodiscard]] constexpr std::span<std::byte> storage() noexcept
    {
        return {reinterpret_cast<std::byte*>(data_), storage_bytes_};
    }

    [[nodiscard]] constexpr std::span<const std::byte> storage() const noexcept
    {
        return {reinterpret_cast<const std::byte*>(data_), storage_bytes_};
    }

    constexpr void reset() noexcept
    {
        if (data_ != nullptr) {
            heap_caps_free(data_);
        }
        data_ = nullptr;
        size_ = 0U;
        storage_bytes_ = 0U;
    }

private:
    T* data_{nullptr};
    std::size_t size_{0U};
    std::size_t storage_bytes_{0U};
};

template <typename T, std::uint32_t Caps, std::size_t Align = alignof(T)>
struct CapsAlloc {
    static_assert(std::has_single_bit(Align), "allocator alignment must be a power of two");

    using value_type = T;
    using is_always_equal = std::true_type;

    constexpr CapsAlloc() noexcept = default;

    template <typename U>
    constexpr CapsAlloc(const CapsAlloc<U, Caps, Align>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(const std::size_t count)
    {
        if (count == 0U) {
            return nullptr;
        }

        if (count > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
            std::abort();
        }

        constexpr auto min_align = alignof(T) > sizeof(void*) ? alignof(T) : sizeof(void*);
        constexpr auto actual_align = Align > min_align ? Align : min_align;

        const auto bytes = count * sizeof(T);
        if constexpr (detail::cache_caps<Caps>) {
            if (!detail::can_round(bytes, actual_align)) {
                std::abort();
            }
        }

        const auto alloc_bytes = detail::allocation_bytes<Caps>(bytes, actual_align);
        void* const storage = heap_caps_aligned_alloc(actual_align, alloc_bytes, Caps);
        if (storage == nullptr) {
            std::abort();
        }

        return static_cast<T*>(storage);
    }

    void deallocate(T* const pointer, std::size_t) noexcept
    {
        heap_caps_free(pointer);
    }

    template <typename U>
    struct rebind {
        using other = CapsAlloc<U, Caps, Align>;
    };
};

template <typename T,
          std::uint32_t Caps,
          std::size_t Align,
          typename U,
          std::uint32_t OtherCaps,
          std::size_t OtherAlign>
[[nodiscard]] constexpr bool operator==(
    const CapsAlloc<T, Caps, Align>&,
    const CapsAlloc<U, OtherCaps, OtherAlign>&) noexcept
{
    return true;
}

template <typename T>
using RamAlloc = CapsAlloc<T, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT>;

template <typename T>
using PsramAlloc = CapsAlloc<T, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT>;

template <typename T>
using DmaAlloc = CapsAlloc<T, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL, arc::cache_line>;

template <typename T>
using SimdAlloc = CapsAlloc<T, MALLOC_CAP_SIMD | MALLOC_CAP_INTERNAL, arc::cache_line>;

template <typename T, std::uint32_t Caps, typename... Args>
[[nodiscard]] inline CapsPtr<T> caps(Args&&... args) noexcept
{
    static_assert(std::is_nothrow_constructible_v<T, Args...>,
                  "capability-aware helpers assume nothrow construction");

    constexpr auto min_align = alignof(T) > sizeof(void*) ? alignof(T) : sizeof(void*);
    void* const storage = heap_caps_aligned_alloc(min_align, sizeof(T), Caps);
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

template <typename T, std::uint32_t Caps, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> capsbuf(const std::size_t count) noexcept
{
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                  "capability buffers require trivial element types");
    static_assert(std::has_single_bit(Align), "buffer alignment must be a power of two");

    if (count == 0U) {
        return {};
    }
    if (count > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
        return {};
    }

    constexpr auto min_align = alignof(T) > sizeof(void*) ? alignof(T) : sizeof(void*);
    constexpr auto actual_align = Align > min_align ? Align : min_align;

    const auto bytes = count * sizeof(T);
    if constexpr (detail::cache_caps<Caps>) {
        if (!detail::can_round(bytes, actual_align)) {
            return {};
        }
    }

    const auto alloc_bytes = detail::allocation_bytes<Caps>(bytes, actual_align);
    void* const storage = heap_caps_aligned_calloc(actual_align, alloc_bytes, 1U, Caps);
    if (storage == nullptr) {
        return {};
    }

    return CapsBuf<T>(static_cast<T*>(storage), count, alloc_bytes);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> inbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, Align>(count);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> psbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, Align>(count);
}

template <typename T, std::size_t Align = arc::cache_line>
[[nodiscard]] inline CapsBuf<T> dmabuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = arc::cache_line>
[[nodiscard]] inline CapsBuf<T> cachebuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_CACHE_ALIGNED | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = arc::cache_line>
[[nodiscard]] inline CapsBuf<T> simdbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_SIMD | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> rtbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_RTCRAM | MALLOC_CAP_8BIT, Align>(count);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> ahbbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_DMA_DESC_AHB | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> axibuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_DMA_DESC_AXI | MALLOC_CAP_INTERNAL, Align>(count);
}

}  // namespace arc
