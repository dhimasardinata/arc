#pragma once

#include <bit>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
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

template <typename T>
class CapsBuf {
public:
    constexpr CapsBuf() noexcept = default;

    constexpr CapsBuf(T* const data, const std::size_t size) noexcept
        : data_(data)
        , size_(size)
    {
    }

    CapsBuf(const CapsBuf&) = delete;
    CapsBuf& operator=(const CapsBuf&) = delete;

    constexpr CapsBuf(CapsBuf&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
    {
        other.data_ = nullptr;
        other.size_ = 0U;
    }

    constexpr CapsBuf& operator=(CapsBuf&& other) noexcept
    {
        if (this != &other) {
            reset();
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0U;
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

    constexpr void reset() noexcept
    {
        if (data_ != nullptr) {
            heap_caps_free(data_);
            data_ = nullptr;
            size_ = 0U;
        }
    }

private:
    T* data_{nullptr};
    std::size_t size_{0U};
};

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

template <typename T, std::uint32_t Caps, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> capsbuf(const std::size_t count) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>, "capability buffers require trivially copyable element types");
    static_assert(std::has_single_bit(Align), "buffer alignment must be a power of two");

    if (count == 0U) {
        return {};
    }

    constexpr auto min_align = alignof(T) > sizeof(void*) ? alignof(T) : sizeof(void*);
    constexpr auto actual_align = Align > min_align ? Align : min_align;

    void* const storage = heap_caps_aligned_calloc(actual_align, count, sizeof(T), Caps);
    if (storage == nullptr) {
        return {};
    }

    return CapsBuf<T>(static_cast<T*>(storage), count);
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

template <typename T, std::size_t Align = CONFIG_CACHE_L1_CACHE_LINE_SIZE>
[[nodiscard]] inline CapsBuf<T> dmabuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = CONFIG_CACHE_L1_CACHE_LINE_SIZE>
[[nodiscard]] inline CapsBuf<T> simdbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_SIMD | MALLOC_CAP_INTERNAL, Align>(count);
}

template <typename T, std::size_t Align = alignof(T)>
[[nodiscard]] inline CapsBuf<T> rtbuf(const std::size_t count) noexcept
{
    return capsbuf<T, MALLOC_CAP_RTCRAM | MALLOC_CAP_8BIT, Align>(count);
}

}  // namespace arc
