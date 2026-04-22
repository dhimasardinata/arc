#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_attr.h"

#include "arc/sdk.hpp"

namespace arc {

template <typename T, std::size_t Capacity>
struct Spsc {
    static_assert(Capacity > 1, "SPSC capacity must be greater than one");
    static_assert(std::has_single_bit(Capacity), "SPSC capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "SPSC payload must be trivially copyable");

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        const auto head = load_relaxed(&head_);
        const auto next = increment(head);
        if (next == load_acquire(&tail_)) {
            return false;
        }

        buffer_[head] = value;
        store_release(&head_, next);
        return true;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
    {
        const auto tail = load_relaxed(&tail_);
        if (tail == load_acquire(&head_)) {
            return false;
        }

        value = buffer_[tail];
        store_release(&tail_, increment(tail));
        return true;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
    {
        return load_acquire(&head_) == load_acquire(&tail_);
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Capacity;
    }

    template <typename Fn>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = Capacity) noexcept(noexcept(fn(value)))
    {
        std::size_t count{};
        while (count < max && try_pop(value)) {
            fn(value);
            ++count;
        }
        return count;
    }

private:
    static constexpr std::uint32_t kMask = static_cast<std::uint32_t>(Capacity - 1U);

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t load_relaxed(
        const std::uint32_t* slot) noexcept
    {
        return __atomic_load_n(slot, __ATOMIC_RELAXED);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t load_acquire(
        const std::uint32_t* slot) noexcept
    {
        return __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void store_release(
        std::uint32_t* slot,
        const std::uint32_t value) noexcept
    {
        __atomic_store_n(slot, value, __ATOMIC_RELEASE);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t increment(
        const std::uint32_t index) noexcept
    {
        return (index + 1U) & kMask;
    }

    std::array<T, Capacity> buffer_{};
    alignas(cache_line) std::uint32_t head_{0};
    alignas(cache_line) std::uint32_t tail_{0};
};

template <typename T, std::size_t Capacity>
using Ring = Spsc<T, Capacity>;

}  // namespace arc
