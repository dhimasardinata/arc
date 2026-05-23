#pragma once

#include <cstdint>

#include "arc/fence.hpp"
#include "arc/soc/target.hpp"

#if ARC_TARGET_ARCH_XTENSA
#include "xtensa/config/core-isa.h"
#include "xtensa/xtruntime.h"
#endif

namespace arc {

#if ARC_TARGET_ARCH_XTENSA
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
#elif ARC_TARGET_ARCH_RISCV
template <unsigned Level = 1U>
struct Mask {
    static_assert(Level <= 1U,
                  "[ARC ERROR] arc::Mask on RISC-V only supports global interrupt masking. Action: use Mask<1>.");

    [[gnu::always_inline]] inline Mask() noexcept
        : state_(clear())
    {
        fence();
    }

    Mask(const Mask&) = delete;
    Mask& operator=(const Mask&) = delete;

    [[gnu::always_inline]] inline ~Mask() noexcept
    {
        fence();
        restore(state_);
    }

    [[nodiscard]] [[gnu::always_inline]] unsigned state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] [[gnu::always_inline]] static unsigned raise() noexcept
    {
        return clear();
    }

    [[gnu::always_inline]] static void restore(const unsigned state) noexcept
    {
#if defined(__riscv)
        if ((state & mie_bit) != 0U) {
            asm volatile("csrsi mstatus, 8" ::: "memory");
        } else {
            asm volatile("csrci mstatus, 8" ::: "memory");
        }
#else
        static_cast<void>(state);
#endif
    }

private:
    static constexpr unsigned mie_bit = 1U << 3U;

    [[nodiscard]] [[gnu::always_inline]] static unsigned clear() noexcept
    {
#if defined(__riscv)
        unsigned state{};
        asm volatile("csrrc %0, mstatus, %1" : "=r"(state) : "r"(mie_bit) : "memory");
        return state;
#else
        return 0U;
#endif
    }

    unsigned state_{0};
};

using Critical = Mask<1U>;
using Silence = Mask<1U>;
#else
template <unsigned Level = 0U>
struct Mask {
    static_assert(soc::never_v<Level>, "arc::Mask has no implementation for this target architecture");
};

using Critical = Mask<0U>;
using Silence = Mask<0U>;
#endif

}  // namespace arc
