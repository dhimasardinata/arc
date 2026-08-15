#pragma once

#include "esp_attr.h"

namespace arc {

IRAM_ATTR [[gnu::always_inline]] inline void compiler_fence() noexcept
{
    __asm__ __volatile__("" ::: "memory");
}

IRAM_ATTR [[gnu::always_inline]] inline void acquire_fence() noexcept
{
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

IRAM_ATTR [[gnu::always_inline]] inline void release_fence() noexcept
{
    __atomic_thread_fence(__ATOMIC_RELEASE);
}

IRAM_ATTR [[gnu::always_inline]] inline void sync_fence() noexcept
{
    __atomic_thread_fence(__ATOMIC_ACQ_REL);
}

IRAM_ATTR [[gnu::always_inline]] inline void fence() noexcept
{
#if defined(__XTENSA__) || defined(__xtensa__)
    __asm__ __volatile__("memw" ::: "memory");
#elif defined(__riscv)
    __asm__ __volatile__("fence rw, rw" ::: "memory");
#else
    compiler_fence();
#endif
}

IRAM_ATTR [[gnu::always_inline]] inline void pause() noexcept
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    __asm__ __volatile__("pause");
#elif defined(__aarch64__) || (defined(__ARM_ARCH) && __ARM_ARCH >= 7)
    __asm__ __volatile__("yield");
#else
    __asm__ __volatile__("nop");
#endif
}

}  // namespace arc
