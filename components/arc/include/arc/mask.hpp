#pragma once

#include <cstdint>

#include "arc/fence.hpp"
#include "esp_attr.h"
#include "xtensa/config/core-isa.h"
#include "xtensa/xtruntime.h"

namespace arc {

template <unsigned Level = XCHAL_EXCM_LEVEL>
struct Mask {
    static_assert(Level <= 15U, "interrupt level must be within Xtensa range");

    IRAM_ATTR Mask() noexcept
        : state_(XTOS_SET_INTLEVEL(Level))
    {
        fence();
    }

    Mask(const Mask&) = delete;
    Mask& operator=(const Mask&) = delete;

    IRAM_ATTR ~Mask() noexcept
    {
        fence();
        XTOS_RESTORE_INTLEVEL(state_);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] unsigned state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static unsigned raise() noexcept
    {
        return XTOS_SET_INTLEVEL(Level);
    }

    IRAM_ATTR [[gnu::always_inline]] static void restore(const unsigned state) noexcept
    {
        XTOS_RESTORE_INTLEVEL(state);
    }

private:
    unsigned state_{0};
};

using Critical = Mask<XCHAL_EXCM_LEVEL>;
using Silence = Mask<15U>;

}  // namespace arc
