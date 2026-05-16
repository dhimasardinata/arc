#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_attr.h"

#include "arc/audit.hpp"
#include "arc/detail/owner.hpp"
#include "arc/spsc.hpp"

namespace arc {

template <typename T, std::size_t Capacity, std::size_t Producers>
struct Fanin {
    using value_type = T;

    static_assert(
        Producers > 0U,
        "[ARC ERROR] arc::Fanin needs at least one producer. "
        "Action: set the Producers template argument to the number of static lanes you will wire.");
    static_assert(
        Capacity > 1U,
        "[ARC ERROR] arc::Fanin lane capacity must be greater than one. "
        "Action: choose a power-of-two lane capacity such as 2, 4, 8, or 16.");
    static_assert(
        std::has_single_bit(Capacity),
        "[ARC ERROR] arc::Fanin lane capacity must be a power of two. "
        "Action: choose 2, 4, 8, 16, and remember usable capacity per lane is Capacity - 1.");
    static_assert(
        std::is_trivially_copyable_v<T>,
        "[ARC ERROR] arc::Fanin payload must be trivially copyable. "
        "Action: use flat structs, integers, enums, or std::array; keep strings, vectors, owning pointers, and virtual objects outside the queue payload.");
    static_assert(
        std::is_copy_assignable_v<T>,
        "[ARC ERROR] arc::Fanin payload must be copy assignable. "
        "Action: keep queued payload fields mutable and flat, or pass stable handles outside the queue payload.");

    template <std::size_t Lane>
    class Producer {
        static_assert(
            Lane < Producers,
            "[ARC ERROR] arc::Fanin producer index is outside the configured producer count. "
            "Action: request producer<Index>() with Index less than Producers.");

    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(Fanin& fan) noexcept
            : fan_(&fan)
        {
        }

        Producer(const Producer&) = delete;
        Producer& operator=(const Producer&) = delete;

        constexpr Producer(Producer&& other) noexcept
            : fan_(std::exchange(other.fan_, nullptr))
        {
        }

        constexpr Producer& operator=(Producer&& other) noexcept
        {
            if (this != &other) {
                fan_ = std::exchange(other.fan_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return fan_ != nullptr;
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
        {
            return fan_ != nullptr && fan_->template try_push<Lane>(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T>)
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t push(
            const std::span<U, Extent> data) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template push<Lane>(data);
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t size() const noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template size<Lane>();
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t space() const noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template space<Lane>();
        }

        [[nodiscard]] static constexpr std::size_t index() noexcept
        {
            return Lane;
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Fanin::cap();
        }

    private:
        Fanin* fan_{};
    };

    class Consumer {
    public:
        constexpr Consumer() noexcept = default;

        explicit constexpr Consumer(Fanin& fan) noexcept
            : fan_(&fan)
        {
        }

        Consumer(const Consumer&) = delete;
        Consumer& operator=(const Consumer&) = delete;

        constexpr Consumer(Consumer&& other) noexcept
            : fan_(std::exchange(other.fan_, nullptr))
        {
        }

        constexpr Consumer& operator=(Consumer&& other) noexcept
        {
            if (this != &other) {
                fan_ = std::exchange(other.fan_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return fan_ != nullptr;
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
        {
            return fan_ != nullptr && fan_->try_pop(value);
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(
            std::size_t& producer,
            T& value) noexcept
        {
            return fan_ != nullptr && fan_->try_pop(producer, value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t pop(
            const std::span<U, Extent> out) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->pop(out);
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
        {
            return fan_ == nullptr || fan_->empty();
        }

        template <typename Fn>
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = (Capacity - 1U) * Producers) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->drain(value, std::forward<Fn>(fn), max);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Fanin::cap();
        }

        [[nodiscard]] static constexpr std::size_t producers() noexcept
        {
            return Fanin::producers();
        }

    private:
        Fanin* fan_{};
    };

    template <std::size_t Lane>
    [[nodiscard]] constexpr Producer<Lane> producer() noexcept
    {
        return Producer<Lane>{*this};
    }

    [[nodiscard]] constexpr Consumer consumer() noexcept
    {
        return Consumer{*this};
    }

    template <std::size_t Lane>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        static_assert(
            Lane < Producers,
            "[ARC ERROR] arc::Fanin producer index is outside the configured producer count. "
            "Action: push to a lane index less than Producers.");
        return lanes_[Lane].try_push(value);
    }

    template <std::size_t Lane, typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T>)
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t push(
        const std::span<U, Extent> data) noexcept
    {
        static_assert(
            Lane < Producers,
            "[ARC ERROR] arc::Fanin producer index is outside the configured producer count. "
            "Action: push to a lane index less than Producers.");
        return lanes_[Lane].push(data);
    }

    template <std::size_t Lane>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t size() const noexcept
    {
        static_assert(
            Lane < Producers,
            "[ARC ERROR] arc::Fanin producer index is outside the configured producer count. "
            "Action: inspect a lane index less than Producers.");
        return lanes_[Lane].size();
    }

    template <std::size_t Lane>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t space() const noexcept
    {
        static_assert(
            Lane < Producers,
            "[ARC ERROR] arc::Fanin producer index is outside the configured producer count. "
            "Action: inspect a lane index less than Producers.");
        return lanes_[Lane].space();
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
    using Lane = Fanin<T, Capacity, Producers>;
    using Checked = Audit<Fanin<T, Capacity, Producers>>;
    using value_type = T;

    template <std::size_t Index>
    class Producer {
        static_assert(
            Index < Producers,
            "[ARC ERROR] arc::Audit<Fanin> producer index is outside the configured producer count. "
            "Action: request producer<Index>() with Index less than Producers.");

    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(Checked& fan) noexcept
            : fan_(&fan)
        {
        }

        Producer(const Producer&) = delete;
        Producer& operator=(const Producer&) = delete;

        constexpr Producer(Producer&& other) noexcept
            : fan_(std::exchange(other.fan_, nullptr))
        {
        }

        constexpr Producer& operator=(Producer&& other) noexcept
        {
            if (this != &other) {
                fan_ = std::exchange(other.fan_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return fan_ != nullptr;
        }

        [[nodiscard]] inline bool try_push(const T& value) noexcept
        {
            return fan_ != nullptr && fan_->template try_push<Index>(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T>)
        [[nodiscard]] inline std::size_t push(const std::span<U, Extent> data) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template push<Index>(data);
        }

        [[nodiscard]] inline std::size_t size() const noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template size<Index>();
        }

        [[nodiscard]] inline std::size_t space() const noexcept
        {
            return fan_ == nullptr ? 0U : fan_->template space<Index>();
        }

        [[nodiscard]] static constexpr std::size_t index() noexcept
        {
            return Index;
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Lane::cap();
        }

    private:
        Checked* fan_{};
    };

    class Consumer {
    public:
        constexpr Consumer() noexcept = default;

        explicit constexpr Consumer(Checked& fan) noexcept
            : fan_(&fan)
        {
        }

        Consumer(const Consumer&) = delete;
        Consumer& operator=(const Consumer&) = delete;

        constexpr Consumer(Consumer&& other) noexcept
            : fan_(std::exchange(other.fan_, nullptr))
        {
        }

        constexpr Consumer& operator=(Consumer&& other) noexcept
        {
            if (this != &other) {
                fan_ = std::exchange(other.fan_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return fan_ != nullptr;
        }

        [[nodiscard]] inline bool try_pop(T& value) noexcept
        {
            return fan_ != nullptr && fan_->try_pop(value);
        }

        [[nodiscard]] inline bool try_pop(
            std::size_t& producer,
            T& value) noexcept
        {
            return fan_ != nullptr && fan_->try_pop(producer, value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
        [[nodiscard]] inline std::size_t pop(const std::span<U, Extent> out) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->pop(out);
        }

        [[nodiscard]] inline bool empty() const noexcept
        {
            return fan_ == nullptr || fan_->empty();
        }

        template <typename Fn>
        [[nodiscard]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = (Capacity - 1U) * Producers) noexcept
        {
            return fan_ == nullptr ? 0U : fan_->drain(value, std::forward<Fn>(fn), max);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Lane::cap();
        }

        [[nodiscard]] static constexpr std::size_t producers() noexcept
        {
            return Lane::producers();
        }

    private:
        Checked* fan_{};
    };

    template <std::size_t ProducerIndex>
    [[nodiscard]] constexpr Producer<ProducerIndex> producer() noexcept
    {
        return Producer<ProducerIndex>{*this};
    }

    [[nodiscard]] constexpr Consumer consumer() noexcept
    {
        return Consumer{*this};
    }

    template <std::size_t Producer>
    [[nodiscard]] inline bool try_push(const T& value) noexcept
    {
        static_assert(
            Producer < Producers,
            "[ARC ERROR] arc::Audit<Fanin> producer index is outside the configured producer count. "
            "Action: push to a lane index less than Producers.");
        producers_[Producer].assert_single("arc::Audit<Fanin> lane must stay single-producer");
        return fan_.template try_push<Producer>(value);
    }

    template <std::size_t Producer, typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T>)
    [[nodiscard]] inline std::size_t push(const std::span<U, Extent> data) noexcept
    {
        static_assert(
            Producer < Producers,
            "[ARC ERROR] arc::Audit<Fanin> producer index is outside the configured producer count. "
            "Action: push to a lane index less than Producers.");
        producers_[Producer].assert_single("arc::Audit<Fanin> lane must stay single-producer");
        return fan_.template push<Producer>(data);
    }

    template <std::size_t Producer>
    [[nodiscard]] inline std::size_t size() const noexcept
    {
        return fan_.template size<Producer>();
    }

    template <std::size_t Producer>
    [[nodiscard]] inline std::size_t space() const noexcept
    {
        return fan_.template space<Producer>();
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
