#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_attr.h"

#include "arc/audit.hpp"
#include "arc/assume.hpp"
#include "arc/detail/owner.hpp"
#include "arc/sdk.hpp"

namespace arc {

template <typename T, std::size_t Capacity>
struct Spsc {
    static_assert(
        Capacity > 1,
        "[ARC ERROR] arc::Spsc capacity must be greater than one. "
        "Action: choose a power-of-two capacity such as 2, 4, 8, or 16; one slot is reserved to distinguish full from empty.");
    static_assert(
        std::has_single_bit(Capacity),
        "[ARC ERROR] arc::Spsc capacity must be a power of two. "
        "Action: choose 2, 4, 8, 16, and remember usable capacity is Capacity - 1.");
    static_assert(
        std::is_trivially_copyable_v<T>,
        "[ARC ERROR] arc::Spsc payload must be trivially copyable. "
        "Action: use flat structs, integers, enums, or std::array; keep strings, vectors, owning pointers, and virtual objects outside the queue payload.");

    class Producer {
    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(Spsc& lane) noexcept
            : lane_(&lane)
        {
        }

        Producer(const Producer&) = delete;
        Producer& operator=(const Producer&) = delete;

        constexpr Producer(Producer&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Producer& operator=(Producer&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_push(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T>)
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t push(
            const std::span<U, Extent> data) noexcept
        {
            return lane_ == nullptr ? 0U : lane_->push(data);
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t space() const noexcept
        {
            return lane_ == nullptr ? 0U : lane_->space();
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Spsc::cap();
        }

    private:
        Spsc* lane_{};
    };

    class Consumer {
    public:
        constexpr Consumer() noexcept = default;

        explicit constexpr Consumer(Spsc& lane) noexcept
            : lane_(&lane)
        {
        }

        Consumer(const Consumer&) = delete;
        Consumer& operator=(const Consumer&) = delete;

        constexpr Consumer(Consumer&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Consumer& operator=(Consumer&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_pop(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t pop(
            const std::span<U, Extent> out) noexcept
        {
            return lane_ == nullptr ? 0U : lane_->pop(out);
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
        {
            return lane_ == nullptr || lane_->empty();
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t size() const noexcept
        {
            return lane_ == nullptr ? 0U : lane_->size();
        }

        template <typename Fn>
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = Capacity - 1U) noexcept(noexcept(fn(value)))
        {
            return lane_ == nullptr ? 0U : lane_->drain(value, std::forward<Fn>(fn), max);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Spsc::cap();
        }

    private:
        Spsc* lane_{};
    };

    struct Endpoints {
        Producer producer;
        Consumer consumer;
    };

    [[nodiscard]] constexpr Producer producer() noexcept
    {
        return Producer{*this};
    }

    [[nodiscard]] constexpr Consumer consumer() noexcept
    {
        return Consumer{*this};
    }

    [[nodiscard]] constexpr Endpoints split() noexcept
    {
        return Endpoints{
            .producer = Producer{*this},
            .consumer = Consumer{*this},
        };
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        const auto head = load_relaxed(&head_);
        const auto next = increment(head);
        if (next == shadow_tail_) {
            shadow_tail_ = load_acquire(&tail_);
            if (next == shadow_tail_) {
                return false;
            }
        }

        assume(head < Capacity);
        buffer_[head] = value;
        store_release(&head_, next);
        return true;
    }

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T>)
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t push(
        const std::span<U, Extent> data) noexcept
    {
        const auto head = load_relaxed(&head_);
        auto room = writable(head, shadow_tail_);
        if (room < data.size()) {
            shadow_tail_ = load_acquire(&tail_);
            room = writable(head, shadow_tail_);
        }
        const auto count = data.size() < room ? data.size() : room;

        if (count != 0U) {
            copy_in(head, data.data(), count);
            store_release(&head_, wrap(head + static_cast<std::uint32_t>(count)));
        }
        return count;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_pop(T& value) noexcept
    {
        const auto tail = load_relaxed(&tail_);
        if (tail == shadow_head_) {
            shadow_head_ = load_acquire(&head_);
            if (tail == shadow_head_) {
                return false;
            }
        }

        assume(tail < Capacity);
        value = buffer_[tail];
        store_release(&tail_, increment(tail));
        return true;
    }

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t pop(
        const std::span<U, Extent> out) noexcept
    {
        const auto tail = load_relaxed(&tail_);
        auto count = readable(shadow_head_, tail);
        if (count < out.size()) {
            shadow_head_ = load_acquire(&head_);
            count = readable(shadow_head_, tail);
        }
        const auto take = out.size() < count ? out.size() : count;

        if (take != 0U) {
            copy_out(out.data(), tail, take);
            store_release(&tail_, wrap(tail + static_cast<std::uint32_t>(take)));
        }
        return take;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
    {
        return load_acquire(&head_) == load_acquire(&tail_);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t size() const noexcept
    {
        return readable(load_acquire(&head_), load_acquire(&tail_));
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t space() const noexcept
    {
        return writable(load_acquire(&head_), load_acquire(&tail_));
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Capacity - 1U;
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Spsc);
    }

    template <typename Fn>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = Capacity - 1U) noexcept(noexcept(fn(value)))
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
        return wrap(index + 1U);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t wrap(
        const std::uint32_t index) noexcept
    {
        assume(kMask != 0U);
        return index & kMask;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t readable(
        const std::uint32_t head,
        const std::uint32_t tail) noexcept
    {
        return (head - tail) & kMask;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t writable(
        const std::uint32_t head,
        const std::uint32_t tail) noexcept
    {
        return (tail - head - 1U) & kMask;
    }

    inline void copy_in(
        const std::uint32_t head,
        const T* const src,
        const std::size_t count) noexcept
    {
        assume(head < Capacity);
        assume(count <= Capacity - 1U);
        const auto first = count < (Capacity - head) ? count : (Capacity - head);
        std::memcpy(buffer_.data() + head, src, first * sizeof(T));
        if (first != count) {
            std::memcpy(buffer_.data(), src + first, (count - first) * sizeof(T));
        }
    }

    inline void copy_out(
        T* const dst,
        const std::uint32_t tail,
        const std::size_t count) noexcept
    {
        assume(tail < Capacity);
        assume(count <= Capacity - 1U);
        const auto first = count < (Capacity - tail) ? count : (Capacity - tail);
        std::memcpy(dst, buffer_.data() + tail, first * sizeof(T));
        if (first != count) {
            std::memcpy(dst + first, buffer_.data(), (count - first) * sizeof(T));
        }
    }

    std::array<T, Capacity> buffer_{};
    alignas(cache_line) std::uint32_t head_{0};
    alignas(cache_line) std::uint32_t tail_{0};
    alignas(cache_line) std::uint32_t shadow_tail_{0};
    alignas(cache_line) std::uint32_t shadow_head_{0};
};

template <typename T, std::size_t Capacity>
using Ring = Spsc<T, Capacity>;

template <typename T, std::size_t Capacity>
struct Audit<Spsc<T, Capacity>> {
    using Lane = Spsc<T, Capacity>;
    using Checked = Audit<Spsc<T, Capacity>>;

    class Producer {
    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(Checked& lane) noexcept
            : lane_(&lane)
        {
        }

        Producer(const Producer&) = delete;
        Producer& operator=(const Producer&) = delete;

        constexpr Producer(Producer&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Producer& operator=(Producer&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] inline bool try_push(const T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_push(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T>)
        [[nodiscard]] inline std::size_t push(const std::span<U, Extent> data) noexcept
        {
            return lane_ == nullptr ? 0U : lane_->push(data);
        }

        [[nodiscard]] inline std::size_t space() const noexcept
        {
            return lane_ == nullptr ? 0U : lane_->space();
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Lane::cap();
        }

    private:
        Checked* lane_{};
    };

    class Consumer {
    public:
        constexpr Consumer() noexcept = default;

        explicit constexpr Consumer(Checked& lane) noexcept
            : lane_(&lane)
        {
        }

        Consumer(const Consumer&) = delete;
        Consumer& operator=(const Consumer&) = delete;

        constexpr Consumer(Consumer&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Consumer& operator=(Consumer&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] inline bool try_pop(T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_pop(value);
        }

        template <typename U, std::size_t Extent>
            requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
        [[nodiscard]] inline std::size_t pop(const std::span<U, Extent> out) noexcept
        {
            return lane_ == nullptr ? 0U : lane_->pop(out);
        }

        [[nodiscard]] inline bool empty() const noexcept
        {
            return lane_ == nullptr || lane_->empty();
        }

        [[nodiscard]] inline std::size_t size() const noexcept
        {
            return lane_ == nullptr ? 0U : lane_->size();
        }

        template <typename Fn>
        [[nodiscard]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = Capacity - 1U) noexcept(noexcept(fn(value)))
        {
            return lane_ == nullptr ? 0U : lane_->drain(value, std::forward<Fn>(fn), max);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return Lane::cap();
        }

    private:
        Checked* lane_{};
    };

    struct Endpoints {
        Producer producer;
        Consumer consumer;
    };

    [[nodiscard]] constexpr Producer producer() noexcept
    {
        return Producer{*this};
    }

    [[nodiscard]] constexpr Consumer consumer() noexcept
    {
        return Consumer{*this};
    }

    [[nodiscard]] constexpr Endpoints split() noexcept
    {
        return Endpoints{
            .producer = Producer{*this},
            .consumer = Consumer{*this},
        };
    }

    [[nodiscard]] inline bool try_push(const T& value) noexcept
    {
        producer_.assert_single("arc::Audit<Spsc> push must stay single-producer");
        return lane_.try_push(value);
    }

    [[nodiscard]] inline bool try_pop(T& value) noexcept
    {
        consumer_.assert_single("arc::Audit<Spsc> pop must stay single-consumer");
        return lane_.try_pop(value);
    }

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T>)
    [[nodiscard]] inline std::size_t push(const std::span<U, Extent> data) noexcept
    {
        producer_.assert_single("arc::Audit<Spsc> push must stay single-producer");
        return lane_.push(data);
    }

    template <typename U, std::size_t Extent>
        requires(std::is_same_v<std::remove_cv_t<U>, T> && !std::is_const_v<U>)
    [[nodiscard]] inline std::size_t pop(const std::span<U, Extent> out) noexcept
    {
        consumer_.assert_single("arc::Audit<Spsc> pop must stay single-consumer");
        return lane_.pop(out);
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return lane_.empty();
    }

    [[nodiscard]] inline std::size_t size() const noexcept
    {
        return lane_.size();
    }

    [[nodiscard]] inline std::size_t space() const noexcept
    {
        return lane_.space();
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Spsc<T, Capacity>::cap();
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Audit<Spsc<T, Capacity>>);
    }

    template <typename Fn>
    [[nodiscard]] inline std::size_t drain(
        T& value,
        Fn&& fn,
        const std::size_t max = Capacity - 1U) noexcept(noexcept(fn(value)))
    {
        std::size_t count{};
        while (count < max && try_pop(value)) {
            fn(value);
            ++count;
        }
        return count;
    }

private:
    Spsc<T, Capacity> lane_{};
    detail::EndpointOwner producer_{};
    detail::EndpointOwner consumer_{};
};

template <typename T, std::size_t Capacity>
using CheckedSpsc = Audit<Spsc<T, Capacity>>;

template <typename T, std::size_t Capacity>
using CheckedRing = Audit<Spsc<T, Capacity>>;

}  // namespace arc
