#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace arc {

enum class Core : BaseType_t {
    core0 = 0,
    core1 = 1,
    any = tskNO_AFFINITY,
};

[[nodiscard]] consteval std::size_t words(const std::size_t bytes) noexcept
{
    return (bytes + sizeof(StackType_t) - 1U) / sizeof(StackType_t);
}

template <std::size_t StackBytes>
struct TaskMem {
    static_assert(StackBytes > 0, "stack size must be non-zero");
    static constexpr std::size_t depth = words(StackBytes);

    std::array<StackType_t, depth> stack{};
    StaticTask_t tcb{};
};

template <std::size_t StackBytes>
[[nodiscard]] inline TaskHandle_t spawn(
    TaskFunction_t entry,
    const char* name,
    void* context,
    const UBaseType_t priority,
    const Core core,
    TaskMem<StackBytes>& mem) noexcept
{
    return xTaskCreateStaticPinnedToCore(
        entry,
        name,
        static_cast<std::uint32_t>(StackBytes),
        context,
        priority,
        mem.stack.data(),
        &mem.tcb,
        static_cast<BaseType_t>(core));
}

}  // namespace arc
