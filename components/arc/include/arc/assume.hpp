#pragma once

#include "esp_attr.h"

namespace arc {

IRAM_ATTR [[gnu::always_inline]] inline void assume(const bool condition) noexcept
{
#if defined(__has_cpp_attribute) && __has_cpp_attribute(assume)
    [[assume(condition)]];
#elif defined(__GNUC__)
    if (!condition) {
        __builtin_unreachable();
    }
#else
    static_cast<void>(condition);
#endif
}

}  // namespace arc
