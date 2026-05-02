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

IRAM_ATTR [[gnu::always_inline]] inline void acq_rel_fence() noexcept
{
    __atomic_thread_fence(__ATOMIC_ACQ_REL);
}

IRAM_ATTR [[gnu::always_inline]] inline void fence() noexcept
{
#if defined(__XTENSA__) || defined(__xtensa__)
    __asm__ __volatile__("memw" ::: "memory");
#else
    compiler_fence();
#endif
}

IRAM_ATTR [[gnu::always_inline]] inline void pause() noexcept
{
    __asm__ __volatile__("nop");
}

}  // namespace arc
