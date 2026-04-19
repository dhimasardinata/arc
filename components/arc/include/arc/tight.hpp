#pragma once

#include <concepts>
#include <cstddef>

#include "esp_attr.h"

#include "arc/mask.hpp"
#include "arc/plane.hpp"
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
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 1)>
struct Tight {
    static_assert(TightWork<Program>, "program must define setup() and step()");
    static_assert(noexcept(Program::step()), "tight step must be noexcept");

    static void boot(const char* tag, const char* name = "tight")
    {
        Plane<Work, StackBytes, void, Bind, Pri>::boot(tag, name);
    }

private:
    struct Work {
        static void setup()
        {
            Program::setup();
        }

        IRAM_ATTR static void run() noexcept
        {
            for (;;) {
                const Guard mask{};
                Program::step();
            }
        }
    };
};

template <typename Program,
          std::size_t StackBytes,
          Core Bind = Core::core1,
          typename Guard = Critical,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 1)>
using Hard = Tight<Program, StackBytes, Bind, Guard, Pri>;

}  // namespace arc
