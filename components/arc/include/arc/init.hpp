#pragma once

#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace arc {

struct Gate {
    explicit Gate(std::uint32_t& state) noexcept
        : state_(&state)
    {
        take(*state_);
    }

    Gate(const Gate&) = delete;
    Gate& operator=(const Gate&) = delete;

    ~Gate()
    {
        drop(*state_);
    }

    static void take(std::uint32_t& state) noexcept
    {
        for (;;) {
            if (try_take(state)) {
                return;
            }

            wait();
        }
    }

    [[nodiscard]] static bool try_take(std::uint32_t& state) noexcept
    {
        auto expected = open;
        return __atomic_compare_exchange_n(
            &state,
            &expected,
            shut,
            false,
            __ATOMIC_ACQ_REL,
            __ATOMIC_ACQUIRE);
    }

    static void drop(std::uint32_t& state) noexcept
    {
        __atomic_store_n(&state, open, __ATOMIC_RELEASE);
    }

    static void wait() noexcept
    {
        configASSERT(!xPortInIsrContext() && "arc::Gate cannot block in ISR context");
        vTaskDelay(1);
    }

private:
    inline constexpr static std::uint32_t open = 0U;
    inline constexpr static std::uint32_t shut = 1U;

    std::uint32_t* state_;
};

struct TryGate {
    explicit TryGate(std::uint32_t& state) noexcept
        : state_(Gate::try_take(state) ? &state : nullptr)
    {
    }

    TryGate(const TryGate&) = delete;
    TryGate& operator=(const TryGate&) = delete;

    TryGate(TryGate&& other) noexcept
        : state_(other.state_)
    {
        other.state_ = nullptr;
    }

    TryGate& operator=(TryGate&& other) noexcept
    {
        if (this != &other) {
            reset();
            state_ = other.state_;
            other.state_ = nullptr;
        }
        return *this;
    }

    ~TryGate()
    {
        reset();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return state_ != nullptr;
    }

    void reset() noexcept
    {
        if (state_ == nullptr) {
            return;
        }

        Gate::drop(*state_);
        state_ = nullptr;
    }

private:
    std::uint32_t* state_{};
};

struct Init {
    [[nodiscard]] static bool begin(std::uint32_t& state) noexcept
    {
        for (;;) {
            auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
            if (current == ready) {
                return false;
            }

            if (current == empty) {
                auto expected = empty;
                if (__atomic_compare_exchange_n(
                        &state,
                        &expected,
                        busy,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return true;
                }
            }

            Gate::wait();
        }
    }

    [[nodiscard]] static bool take(std::uint32_t& state) noexcept
    {
        for (;;) {
            auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
            if (current == empty) {
                return false;
            }

            if (current == ready) {
                auto expected = ready;
                if (__atomic_compare_exchange_n(
                        &state,
                        &expected,
                        busy,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return true;
                }
            }

            Gate::wait();
        }
    }

    static void pass(std::uint32_t& state) noexcept
    {
        __atomic_store_n(&state, ready, __ATOMIC_RELEASE);
    }

    static void fail(std::uint32_t& state) noexcept
    {
        __atomic_store_n(&state, empty, __ATOMIC_RELEASE);
    }

    [[nodiscard]] static bool is_empty(const std::uint32_t& state) noexcept
    {
        return load(state) == empty;
    }

    [[nodiscard]] static bool is_busy(const std::uint32_t& state) noexcept
    {
        return load(state) == busy;
    }

    [[nodiscard]] static bool is_ready(const std::uint32_t& state) noexcept
    {
        return load(state) == ready;
    }

private:
    [[nodiscard]] static std::uint32_t load(const std::uint32_t& state) noexcept
    {
        return __atomic_load_n(&state, __ATOMIC_ACQUIRE);
    }

    inline constexpr static std::uint32_t empty = 0U;
    inline constexpr static std::uint32_t busy = 1U;
    inline constexpr static std::uint32_t ready = 2U;
};

namespace detail {

template <typename Machine>
struct InitTxnFor {
    InitTxnFor() = default;

    InitTxnFor(const InitTxnFor&) = delete;
    InitTxnFor& operator=(const InitTxnFor&) = delete;

    InitTxnFor(InitTxnFor&& other) noexcept
        : state_(other.state_)
    {
        other.state_ = nullptr;
    }

    InitTxnFor& operator=(InitTxnFor&& other) noexcept
    {
        if (this != &other) {
            rollback();
            state_ = other.state_;
            other.state_ = nullptr;
        }
        return *this;
    }

    ~InitTxnFor()
    {
        rollback();
    }

    [[nodiscard]] static InitTxnFor begin(std::uint32_t& state) noexcept
    {
        if (!Machine::begin(state)) {
            return {};
        }
        return InitTxnFor(state);
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return state_ != nullptr;
    }

    [[nodiscard]] bool pass() noexcept
    {
        if (state_ == nullptr) {
            return false;
        }

        Machine::pass(*state_);
        state_ = nullptr;
        return true;
    }

    [[nodiscard]] bool fail() noexcept
    {
        if (state_ == nullptr) {
            return false;
        }

        Machine::fail(*state_);
        state_ = nullptr;
        return true;
    }

private:
    explicit InitTxnFor(std::uint32_t& state) noexcept
        : state_(&state)
    {
    }

    void rollback() noexcept
    {
        if (state_ == nullptr) {
            return;
        }

        Machine::fail(*state_);
        state_ = nullptr;
    }

    std::uint32_t* state_{};
};

}  // namespace detail

using InitTxn = detail::InitTxnFor<Init>;

struct RefInit {
    [[nodiscard]] static bool begin(std::uint32_t& state) noexcept
    {
        for (;;) {
            auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
            if (current == empty) {
                auto expected = empty;
                if (__atomic_compare_exchange_n(
                        &state,
                        &expected,
                        busy,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return true;
                }
                continue;
            }

            if (current != busy) {
                configASSERT(current < max_refs && "arc::RefInit reference count exhausted");
                if (__atomic_compare_exchange_n(
                        &state,
                        &current,
                        current + 1U,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return false;
                }
                continue;
            }

            Gate::wait();
        }
    }

    [[nodiscard]] static bool take(std::uint32_t& state) noexcept
    {
        for (;;) {
            auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
            if (current == empty) {
                return false;
            }

            if (current != busy) {
                configASSERT(current < max_refs && "arc::RefInit reference count exhausted");
                if (__atomic_compare_exchange_n(
                        &state,
                        &current,
                        current + 1U,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return true;
                }
                continue;
            }

            Gate::wait();
        }
    }

    static void pass(std::uint32_t& state) noexcept
    {
        __atomic_store_n(&state, 1U, __ATOMIC_RELEASE);
    }

    static void fail(std::uint32_t& state) noexcept
    {
        __atomic_store_n(&state, empty, __ATOMIC_RELEASE);
    }

    [[nodiscard]] static bool drop(std::uint32_t& state) noexcept
    {
        for (;;) {
            auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
            configASSERT(current != empty && current != busy && "arc::RefInit drop without a live reference");

            if (current == 1U) {
                auto expected = 1U;
                if (__atomic_compare_exchange_n(
                        &state,
                        &expected,
                        busy,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE)) {
                    return true;
                }
                continue;
            }

            if (__atomic_compare_exchange_n(
                    &state,
                    &current,
                    current - 1U,
                    false,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_ACQUIRE)) {
                return false;
            }
        }
    }

    [[nodiscard]] static std::uint32_t refs(const std::uint32_t& state) noexcept
    {
        const auto current = load(state);
        return current == busy ? 0U : current;
    }

    [[nodiscard]] static bool is_empty(const std::uint32_t& state) noexcept
    {
        return load(state) == empty;
    }

    [[nodiscard]] static bool is_busy(const std::uint32_t& state) noexcept
    {
        return load(state) == busy;
    }

    [[nodiscard]] static bool is_ready(const std::uint32_t& state) noexcept
    {
        const auto current = load(state);
        return current != empty && current != busy;
    }

private:
    [[nodiscard]] static std::uint32_t load(const std::uint32_t& state) noexcept
    {
        return __atomic_load_n(&state, __ATOMIC_ACQUIRE);
    }

    inline constexpr static std::uint32_t empty = 0U;
    inline constexpr static std::uint32_t busy = ~std::uint32_t{0};
    inline constexpr static std::uint32_t max_refs = busy - 1U;
};

using RefInitTxn = detail::InitTxnFor<RefInit>;

struct RefLease {
    RefLease() = default;

    RefLease(const RefLease&) = delete;
    RefLease& operator=(const RefLease&) = delete;

    RefLease(RefLease&& other) noexcept
        : state_(other.state_)
    {
        other.state_ = nullptr;
    }

    RefLease& operator=(RefLease&& other) noexcept
    {
        if (this != &other) {
            abandon();
            state_ = other.state_;
            other.state_ = nullptr;
        }
        return *this;
    }

    ~RefLease()
    {
        abandon();
    }

    [[nodiscard]] static RefLease take(std::uint32_t& state) noexcept
    {
        if (!RefInit::take(state)) {
            return {};
        }
        return RefLease(state);
    }

    [[nodiscard]] static RefLease adopt(std::uint32_t& state) noexcept
    {
        return RefLease(state);
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return state_ != nullptr;
    }

    [[nodiscard]] bool release() noexcept
    {
        if (state_ == nullptr) {
            return false;
        }

        auto& state = *state_;
        state_ = nullptr;
        return RefInit::drop(state);
    }

private:
    explicit RefLease(std::uint32_t& state) noexcept
        : state_(&state)
    {
    }

    void abandon() noexcept
    {
        if (state_ == nullptr) {
            return;
        }

        auto& state = *state_;
        state_ = nullptr;
        const auto last = RefInit::drop(state);
        if (last) {
            RefInit::fail(state);
            configASSERT(false && "arc::RefLease final owner must call release() and tear down");
        }
    }

    std::uint32_t* state_{};
};

}  // namespace arc
