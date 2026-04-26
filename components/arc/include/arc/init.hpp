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
            auto expected = open;
            if (__atomic_compare_exchange_n(
                    &state,
                    &expected,
                    shut,
                    false,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_ACQUIRE)) {
                return;
            }

            wait();
        }
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

private:
    inline constexpr static std::uint32_t empty = 0U;
    inline constexpr static std::uint32_t busy = 1U;
    inline constexpr static std::uint32_t ready = 2U;
};

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
        const auto current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);
        return current == busy ? 0U : current;
    }

private:
    inline constexpr static std::uint32_t empty = 0U;
    inline constexpr static std::uint32_t busy = ~std::uint32_t{0};
    inline constexpr static std::uint32_t max_refs = busy - 1U;
};

}  // namespace arc
