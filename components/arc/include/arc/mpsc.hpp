#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "esp_attr.h"

#include "arc/audit.hpp"
#include "arc/detail/owner.hpp"
#include "arc/fence.hpp"
#include "arc/sdk.hpp"

namespace arc {

namespace detail {

template <typename T>
inline constexpr std::size_t dense_mpsc_align =
    alignof(T) > alignof(std::uint32_t) ? alignof(T) : alignof(std::uint32_t);

template <typename T, std::size_t Capacity, std::size_t CellAlign>
struct MpscImpl {
    using value_type = T;

    static_assert(
        Capacity > 1,
        "[ARC ERROR] arc::Mpsc capacity must be greater than one. "
        "Action: choose a power-of-two capacity such as 2, 4, 8, or 16.");
    static_assert(
        std::has_single_bit(Capacity),
        "[ARC ERROR] arc::Mpsc capacity must be a power of two. "
        "Action: choose 2, 4, 8, 16, or another power-of-two queue depth.");
    static_assert(
        Capacity <= (std::size_t{1} << 31U),
        "[ARC ERROR] arc::Mpsc capacity is too large for the sequence arithmetic. "
        "Action: reduce the queue depth below 2^31.");
    static_assert(
        std::is_trivially_copyable_v<T>,
        "[ARC ERROR] arc::Mpsc payload must be trivially copyable. "
        "Action: use flat structs, integers, enums, or std::array; keep strings, vectors, owning pointers, and virtual objects outside the queue payload.");
    static_assert(
        std::is_copy_assignable_v<T>,
        "[ARC ERROR] arc::Mpsc payload must be copy assignable. "
        "Action: keep queued payload fields mutable and flat, or pass stable handles outside the queue payload.");
    static_assert(
        CellAlign >= alignof(std::uint32_t),
        "[ARC ERROR] arc::Mpsc cell alignment must fit the sequence word. "
        "Action: use the default arc::Mpsc or keep custom cell alignment at least alignof(std::uint32_t).");
    static_assert(
        (CellAlign & (CellAlign - 1U)) == 0U,
        "[ARC ERROR] arc::Mpsc cell alignment must be a power of two. "
        "Action: choose a natural power-of-two alignment such as 4, 8, 16, 32, or arc::cache_line.");

    class Producer {
    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(MpscImpl& lane) noexcept
            : lane_(&lane)
        {
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_push(value);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return MpscImpl::cap();
        }

    private:
        MpscImpl* lane_{};
    };

    class Consumer {
    public:
        constexpr Consumer() noexcept = default;

        explicit constexpr Consumer(MpscImpl& lane) noexcept
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

        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool empty() const noexcept
        {
            return lane_ == nullptr || lane_->empty();
        }

        template <typename Fn>
        [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = Capacity) noexcept(noexcept(fn(value)))
        {
            return lane_ == nullptr ? 0U : lane_->drain(value, std::forward<Fn>(fn), max);
        }

        [[nodiscard]] static constexpr std::size_t cap() noexcept
        {
            return MpscImpl::cap();
        }

    private:
        MpscImpl* lane_{};
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

    constexpr MpscImpl() noexcept
    {
        for (std::size_t i = 0; i < Capacity; ++i) {
            buffer_[i].seq = static_cast<std::uint32_t>(i);
        }
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool try_push(const T& value) noexcept
    {
        Cell* cell = nullptr;
        auto head = load_relaxed(&head_);
        Backoff backoff{};

        for (;;) {
            cell = &buffer_[head & kMask];
            const auto seq = load_acquire(&cell->seq);
            const auto diff = delta(seq, head);

            if (diff == 0) {
                if (compare_exchange(&head_, &head, head + 1U)) {
                    break;
                }
                backoff.wait();
            } else if (diff < 0) {
                return false;
            } else {
                head = load_relaxed(&head_);
                backoff.wait();
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

    [[nodiscard]] static constexpr std::size_t cell_align() noexcept
    {
        return CellAlign;
    }

    [[nodiscard]] static constexpr std::size_t cell_bytes() noexcept
    {
        return sizeof(Cell);
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(MpscImpl);
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
        alignas(CellAlign) std::uint32_t seq{};
        T value{};
    };

    static constexpr std::uint32_t kMask = static_cast<std::uint32_t>(Capacity - 1U);
    static constexpr std::uint32_t kBackoffMax = 16U;

    struct Backoff {
        std::uint32_t spins{1U};

        IRAM_ATTR [[gnu::always_inline]] inline void wait() noexcept
        {
            for (std::uint32_t i = 0; i < spins; ++i) {
                pause();
            }
            if (spins < kBackoffMax) {
                spins <<= 1U;
            }
        }
    };

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

}  // namespace detail

template <typename T, std::size_t Capacity>
using Mpsc = detail::MpscImpl<T, Capacity, cache_line>;

template <typename T, std::size_t Capacity>
using DenseMpsc = detail::MpscImpl<T, Capacity, detail::dense_mpsc_align<T>>;

template <typename T, std::size_t Capacity>
using Mux = Mpsc<T, Capacity>;

template <typename T, std::size_t Capacity>
using DenseMux = DenseMpsc<T, Capacity>;

template <typename T, std::size_t Capacity, std::size_t CellAlign>
struct Audit<detail::MpscImpl<T, Capacity, CellAlign>> {
    using Lane = detail::MpscImpl<T, Capacity, CellAlign>;
    using Checked = Audit<detail::MpscImpl<T, Capacity, CellAlign>>;
    using value_type = T;

    class Producer {
    public:
        constexpr Producer() noexcept = default;

        explicit constexpr Producer(Checked& lane) noexcept
            : lane_(&lane)
        {
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] inline bool try_push(const T& value) noexcept
        {
            return lane_ != nullptr && lane_->try_push(value);
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

        [[nodiscard]] inline bool empty() const noexcept
        {
            return lane_ == nullptr || lane_->empty();
        }

        template <typename Fn>
        [[nodiscard]] inline std::size_t drain(
            T& value,
            Fn&& fn,
            const std::size_t max = Capacity) noexcept(noexcept(fn(value)))
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
        return lane_.try_push(value);
    }

    [[nodiscard]] inline bool try_pop(T& value) noexcept
    {
        consumer_.assert_single("arc::Audit<Mpsc> pop must stay single-consumer");
        return lane_.try_pop(value);
    }

    [[nodiscard]] inline bool empty() const noexcept
    {
        return lane_.empty();
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Lane::cap();
    }

    [[nodiscard]] static constexpr std::size_t cell_align() noexcept
    {
        return Lane::cell_align();
    }

    [[nodiscard]] static constexpr std::size_t cell_bytes() noexcept
    {
        return Lane::cell_bytes();
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Audit<detail::MpscImpl<T, Capacity, CellAlign>>);
    }

    template <typename Fn>
    [[nodiscard]] inline std::size_t drain(
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
    Lane lane_{};
    detail::EndpointOwner consumer_{};
};

template <typename T, std::size_t Capacity>
using CompactMpsc = DenseMpsc<T, Capacity>;

template <typename T, std::size_t Capacity>
using CompactMux = DenseMux<T, Capacity>;

template <typename T, std::size_t Capacity>
using CheckedMpsc = Audit<Mpsc<T, Capacity>>;

template <typename T, std::size_t Capacity>
using CheckedCompactMpsc = Audit<DenseMpsc<T, Capacity>>;

}  // namespace arc
