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
struct Mpsc {
    static_assert(Capacity > 1, "MPSC capacity must be greater than one");
    static_assert(std::has_single_bit(Capacity), "MPSC capacity must be a power of two");
    static_assert(Capacity <= (std::size_t{1} << 31U), "MPSC capacity is too large");
    static_assert(std::is_trivially_copyable_v<T>, "MPSC payload must be trivially copyable");

    constexpr Mpsc() noexcept
    {
        for (std::size_t i = 0; i < Capacity; ++i) {
            buffer_[i].seq = static_cast<std::uint32_t>(i);
        }
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        Cell* cell = nullptr;
        auto head = load_relaxed(&head_);

        for (;;) {
            cell = &buffer_[head & kMask];
            const auto seq = load_acquire(&cell->seq);
            const auto diff = delta(seq, head);

            if (diff == 0) {
                if (compare_exchange(&head_, &head, head + 1U)) {
                    break;
                }
            } else if (diff < 0) {
                return false;
            } else {
                head = load_relaxed(&head_);
            }
        }

        cell->value = value;
        store_release(&cell->seq, head + 1U);
        return true;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
    {
        const auto tail = load_relaxed(&tail_);
        auto& cell = buffer_[tail & kMask];
        const auto seq = load_acquire(&cell.seq);
        const auto diff = delta(seq, tail + 1U);

        if (diff != 0) {
            return false;
        }

        value = cell.value;
        store_relaxed(&tail_, tail + 1U);
        store_release(&cell.seq, tail + static_cast<std::uint32_t>(Capacity));
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
    struct Cell {
        alignas(cache_line) std::uint32_t seq{};
        T value{};
    };

    static constexpr std::uint32_t kMask = static_cast<std::uint32_t>(Capacity - 1U);

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::int32_t delta(
        const std::uint32_t value,
        const std::uint32_t target) noexcept
    {
        return std::bit_cast<std::int32_t>(value - target);
    }

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

    IRAM_ATTR [[gnu::always_inline]] static inline void store_relaxed(
        std::uint32_t* slot,
        const std::uint32_t value) noexcept
    {
        __atomic_store_n(slot, value, __ATOMIC_RELAXED);
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void store_release(
        std::uint32_t* slot,
        const std::uint32_t value) noexcept
    {
        __atomic_store_n(slot, value, __ATOMIC_RELEASE);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline bool compare_exchange(
        std::uint32_t* slot,
        std::uint32_t* expected,
        const std::uint32_t value) noexcept
    {
        return __atomic_compare_exchange_n(
            slot,
            expected,
            value,
            false,
            __ATOMIC_ACQ_REL,
            __ATOMIC_RELAXED);
    }

    std::array<Cell, Capacity> buffer_{};
    alignas(cache_line) std::uint32_t head_{0};
    alignas(cache_line) std::uint32_t tail_{0};
};

template <typename T, std::size_t Capacity>
using Mux = Mpsc<T, Capacity>;

}  // namespace arc
