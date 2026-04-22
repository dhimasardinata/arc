#pragma once

#include <cstdint>

#include "arc/fence.hpp"
#include "xtensa/config/core-isa.h"
#include "xtensa/xtruntime.h"

namespace arc {

template <unsigned Level = XCHAL_EXCM_LEVEL>
struct Mask {
    static_assert(Level <= 15U, "interrupt level must be within Xtensa range");

    [[gnu::always_inline]] inline Mask() noexcept
        : state_(set())
    {
        fence();
    }

    Mask(const Mask&) = delete;
    Mask& operator=(const Mask&) = delete;

    [[gnu::always_inline]] inline ~Mask() noexcept
    {
        fence();
        XTOS_RESTORE_INTLEVEL(state_);
    }

    [[nodiscard]] [[gnu::always_inline]] unsigned state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] [[gnu::always_inline]] static unsigned raise() noexcept
    {
        return set();
    }

    [[gnu::always_inline]] static void restore(const unsigned state) noexcept
    {
        XTOS_RESTORE_INTLEVEL(state);
    }

private:
    [[nodiscard]] [[gnu::always_inline]] static unsigned set() noexcept
    {
        if constexpr (Level == 0U) {
            return XTOS_SET_INTLEVEL(0);
        } else if constexpr (Level == 1U) {
            return XTOS_SET_INTLEVEL(1);
        } else if constexpr (Level == 2U) {
            return XTOS_SET_INTLEVEL(2);
        } else if constexpr (Level == 3U) {
            return XTOS_SET_INTLEVEL(3);
        } else if constexpr (Level == 4U) {
            return XTOS_SET_INTLEVEL(4);
        } else if constexpr (Level == 5U) {
            return XTOS_SET_INTLEVEL(5);
        } else if constexpr (Level == 6U) {
            return XTOS_SET_INTLEVEL(6);
        } else if constexpr (Level == 7U) {
            return XTOS_SET_INTLEVEL(7);
        } else if constexpr (Level == 8U) {
            return XTOS_SET_INTLEVEL(8);
        } else if constexpr (Level == 9U) {
            return XTOS_SET_INTLEVEL(9);
        } else if constexpr (Level == 10U) {
            return XTOS_SET_INTLEVEL(10);
        } else if constexpr (Level == 11U) {
            return XTOS_SET_INTLEVEL(11);
        } else if constexpr (Level == 12U) {
            return XTOS_SET_INTLEVEL(12);
        } else if constexpr (Level == 13U) {
            return XTOS_SET_INTLEVEL(13);
        } else if constexpr (Level == 14U) {
            return XTOS_SET_INTLEVEL(14);
        } else {
            return XTOS_SET_INTLEVEL(15);
        }
    }

    unsigned state_{0};
};

using Critical = Mask<XCHAL_EXCM_LEVEL>;
using Silence = Mask<15U>;

}  // namespace arc
