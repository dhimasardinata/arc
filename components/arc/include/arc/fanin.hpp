#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_attr.h"

#include "arc/audit.hpp"
#include "arc/detail/owner.hpp"
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

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t pop(
        const std::span<U, Extent> out) noexcept
    {
        std::size_t count{};
        while (count < out.size()) {
            const auto start = cursor_;
            auto moved = false;
            for (std::size_t offset = 0; offset < Producers && count < out.size(); ++offset) {
                const auto index = wrap(start + offset);
                const auto got = lanes_[index].pop(out.subspan(count));
                if (got != 0U) {
                    count += got;
                    cursor_ = wrap(index + 1U);
                    moved = true;
                }
            }
            if (!moved) {
                break;
            }
        }
        return count;
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
        return Spsc<T, Capacity>::cap();
    }

    [[nodiscard]] static constexpr std::size_t producers() noexcept
    {
        return Producers;
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Fanin);
    }

    template <typename Fn>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = (Capacity - 1U) * Producers) noexcept
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

template <typename T, std::size_t Capacity, std::size_t Producers>
struct Audit<Fanin<T, Capacity, Producers>> {
    template <std::size_t Producer>
    [[nodiscard]] inline bool try_push(const T& value) noexcept
    {
        static_assert(Producer < Producers, "invalid fanin producer");
        producers_[Producer].assert_single("arc::Audit<Fanin> lane must stay single-producer");
        return fan_.template try_push<Producer>(value);
    }

    [[nodiscard]] inline bool try_pop(T& value) noexcept
    {
        consumer_.assert_single("arc::Audit<Fanin> pop must stay single-consumer");
        return fan_.try_pop(value);
    }

    [[nodiscard]] inline bool try_pop(
        std::size_t& producer,
        T& value) noexcept
    {
        consumer_.assert_single("arc::Audit<Fanin> pop must stay single-consumer");
        return fan_.try_pop(producer, value);
    }

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
    [[nodiscard]] inline std::size_t pop(const std::span<U, Extent> out) noexcept
    {
        consumer_.assert_single("arc::Audit<Fanin> pop must stay single-consumer");
        return fan_.pop(out);
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return fan_.empty();
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Fanin<T, Capacity, Producers>::cap();
    }

    [[nodiscard]] static constexpr std::size_t producers() noexcept
    {
        return Fanin<T, Capacity, Producers>::producers();
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Audit<Fanin<T, Capacity, Producers>>);
    }

    template <typename Fn>
    [[nodiscard]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = (Capacity - 1U) * Producers) noexcept
    {
        std::size_t count{};
        std::size_t producer{};
        while (count < max && try_pop(producer, value)) {
            if constexpr (requires { fn(producer, value); }) {
                fn(producer, value);
            } else {
                fn(value);
            }
            ++count;
        }
        return count;
    }

private:
    Fanin<T, Capacity, Producers> fan_{};
    std::array<detail::EndpointOwner, Producers> producers_{};
    detail::EndpointOwner consumer_{};
};

template <typename T, std::size_t Capacity, std::size_t Producers>
using CheckedFanin = Audit<Fanin<T, Capacity, Producers>>;

}  // namespace arc
