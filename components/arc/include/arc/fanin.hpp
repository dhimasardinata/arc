#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_attr.h"

#include "arc/spsc.hpp"

namespace arc {

template <typename T, std::size_t Capacity, std::size_t Producers>
struct Fanin {
    static_assert(Producers > 0U, "fanin needs at least one producer");
    static_assert(Capacity > 1U, "fanin lane capacity must be greater than one");
    static_assert(std::has_single_bit(Capacity), "fanin lane capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "fanin payload must be trivially copyable");

    template <std::size_t Producer>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        static_assert(Producer < Producers, "invalid fanin producer");
        return lanes_[Producer].try_push(value);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
    {
        std::size_t producer{};
        return try_pop(producer, value);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(
        std::size_t& producer,
        T& value) noexcept
    {
        const auto start = cursor_;
        for (std::size_t offset = 0; offset < Producers; ++offset) {
            const auto index = wrap(start + offset);
            if (lanes_[index].try_pop(value)) {
                producer = index;
                cursor_ = wrap(index + 1U);
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
    {
        for (const auto& lane : lanes_) {
            if (!lane.empty()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Capacity;
    }

    [[nodiscard]] static constexpr std::size_t producers() noexcept
    {
        return Producers;
    }

    template <typename Fn>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = Capacity * Producers) noexcept
    {
        std::size_t count{};
        std::size_t producer{};
        while (count < max && try_pop(producer, value)) {
            call(fn, producer, value);
            ++count;
        }
        return count;
    }

private:
    [[nodiscard]] static constexpr std::size_t wrap(const std::size_t value) noexcept
    {
        return value >= Producers ? value - Producers : value;
    }

    template <typename Fn>
    static void call(Fn&& fn, const std::size_t producer, T& value) noexcept
    {
        if constexpr (requires { fn(producer, value); }) {
            fn(producer, value);
        } else {
            static_cast<void>(producer);
            fn(value);
        }
    }

    std::array<Spsc<T, Capacity>, Producers> lanes_{};
    std::size_t cursor_{};
};

}  // namespace arc
