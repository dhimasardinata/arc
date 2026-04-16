#pragma once

#include <concepts>
#include <cstddef>

#include "esp_attr.h"

#include "arc/plane.hpp"
#include "arc/task.hpp"

namespace arc {

template <typename Program>
concept SketchWork = requires {
    { Program::setup() } -> std::same_as<void>;
    { Program::loop() } -> std::same_as<void>;
};

template <typename Program,
          std::size_t StackBytes,
          Core Bind = Core::core1,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 2)>
struct Sketch {
    static_assert(SketchWork<Program>, "program must define setup() and loop()");
    static_assert(noexcept(Program::loop()), "program loop must be noexcept");

    static void boot(const char* tag, const char* name = "app")
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
                Program::loop();
            }
        }
    };
};

template <typename Program,
          std::size_t StackBytes,
          Core Bind = Core::core1,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 2)>
using App = Sketch<Program, StackBytes, Bind, Pri>;

}  // namespace arc
