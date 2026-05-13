#pragma once

#include <concepts>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "arc/soc/target.hpp"
#include "arc/stack.hpp"

namespace arc {

enum class Core : BaseType_t {
    core0 = 0,
    core1 = 1,
    any = tskNO_AFFINITY,
};

enum class CoreRole : std::uint8_t {
    ctrl,
    det,
};

template <typename Target = soc::Target>
struct CoreMap {
    static constexpr bool dual = Target::cores > 1U;
    static constexpr Core ctrl = Core::core0;
    static constexpr Core det = dual ? Core::core1 : Core::core0;
    static constexpr bool simd = Target::simd;
    static constexpr bool riscv = Target::Arch::csr;
    static constexpr bool exp = Target::experimental;

    [[nodiscard]] static consteval Core core(const CoreRole role) noexcept
    {
        return role == CoreRole::det ? det : ctrl;
    }
};

[[nodiscard]] consteval std::size_t words(const std::size_t bytes) noexcept
{
    return (bytes + sizeof(StackType_t) - 1U) / sizeof(StackType_t);
}

template <auto* Object>
using StaticObject = std::remove_cv_t<std::remove_reference_t<decltype(*Object)>>;

template <auto* Object, typename T>
concept StaticTaskState =
    Object != nullptr &&
    std::same_as<StaticObject<Object>, T> &&
    !std::is_const_v<std::remove_pointer_t<decltype(Object)>>;

template <std::size_t StackBytes, std::size_t RequiredBytes = stack::task_floor>
struct TaskMem {
    static_assert(StackBytes > 0, "stack size must be non-zero");
    static_assert(
        stack::fits<StackBytes, RequiredBytes>(),
        "static task stack is smaller than its compile-time Arc stack budget");
    static constexpr std::size_t depth = words(StackBytes);
    static constexpr std::size_t bytes = StackBytes;
    static constexpr std::size_t required = RequiredBytes;

    alignas(portBYTE_ALIGNMENT) std::array<StackType_t, depth> stack{};
    StaticTask_t tcb{};
};

template <std::size_t StackBytes, std::size_t RequiredBytes>
[[nodiscard]] inline TaskHandle_t spawn(
    TaskFunction_t entry,
    const char* name,
    void* context,
    const UBaseType_t priority,
    const Core core,
    TaskMem<StackBytes, RequiredBytes>& mem) noexcept
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

[[noreturn]] inline void park_task() noexcept
{
    vTaskSuspend(nullptr);
    for (;;) {
        vTaskSuspend(nullptr);
    }
}

}  // namespace arc
