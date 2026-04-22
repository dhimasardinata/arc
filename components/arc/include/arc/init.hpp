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

}  // namespace arc
