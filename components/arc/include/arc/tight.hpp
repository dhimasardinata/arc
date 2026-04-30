#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

#include "esp_attr.h"

#include "arc/clock.hpp"
#include "arc/mask.hpp"
#include "arc/plane.hpp"
#include "arc/stack.hpp"
#include "arc/task.hpp"

namespace arc {

template <typename Program>
concept TightWork = requires {
    { Program::setup() } -> std::same_as<void>;
    { Program::step() } -> std::same_as<void>;
};

template <typename Program,
          std::size_t StackBytes,
          Core Bind = Core::core1,
          typename Guard = Critical,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 1),
          esp_cpu_cycle_count_t Budget = 0U>
struct Tight {
    static_assert(TightWork<Program>, "program must define setup() and step()");
    static_assert(noexcept(Program::step()), "tight step must be noexcept");
    static constexpr std::size_t stack_bytes = StackBytes;
    static constexpr std::size_t stack_required = stack::required<Program>();
    static_assert(
        stack::fits<stack_bytes, stack_required>(),
        "Tight StackBytes is smaller than Program::stack_bytes compile-time budget");

    static void boot(const char* tag, const char* name = "tight")
    {
        Plane<Work, StackBytes, void, Bind, Pri>::boot(tag, name);
    }

    [[nodiscard]] static std::uint32_t overruns() noexcept
    {
        return __atomic_load_n(&overrun_count, __ATOMIC_RELAXED);
    }

private:
    constinit static inline std::uint32_t overrun_count{};

    struct Work {
        static constexpr std::size_t stack_bytes = stack::required<Program>();

        static void setup()
        {
            Program::setup();
        }

        IRAM_ATTR static void run() noexcept
        {
            for (;;) {
                if constexpr (Budget == 0U) {
                    const Guard mask{};
                    Program::step();
                } else {
                    const auto start = Clock::tick();
                    {
                        const Guard mask{};
                        Program::step();
                    }
                    const auto cycles = static_cast<esp_cpu_cycle_count_t>(Clock::tick() - start);
                    if (cycles > Budget) {
                        __atomic_add_fetch(&overrun_count, 1U, __ATOMIC_RELAXED);
                        if constexpr (requires(esp_cpu_cycle_count_t spent) {
                                          { Program::overrun(spent) } noexcept -> std::same_as<void>;
                                      }) {
                            Program::overrun(cycles);
                        }
                    }
                }
            }
        }
    };
};

template <typename Program,
          std::size_t StackBytes,
          Core Bind = Core::core1,
          typename Guard = Critical,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 1),
          esp_cpu_cycle_count_t Budget = 0U>
using Hard = Tight<Program, StackBytes, Bind, Guard, Pri, Budget>;

}  // namespace arc
