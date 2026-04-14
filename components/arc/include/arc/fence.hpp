#pragma once

#include "esp_attr.h"

namespace arc {

IRAM_ATTR [[gnu::always_inline]] inline void fence() noexcept
{
    __asm__ __volatile__("" ::: "memory");
}

IRAM_ATTR [[gnu::always_inline]] inline void pause() noexcept
{
    __asm__ __volatile__("nop");
}

}  // namespace arc
