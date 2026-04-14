#pragma once

#include <cstdint>

#include "esp_attr.h"

#include "arc/clock.hpp"
#include "arc/dio.hpp"
#include "arc/fence.hpp"
#include "arc/plane.hpp"
#include "cfg.hpp"
#include "lane.hpp"
#include "net.hpp"

namespace app {

using Led = arc::Dio<cfg::led, 0>;

struct Loop {
    static void setup(Ctx&) { Led::out(); Led::template write<false>(); }

    IRAM_ATTR static void run(Ctx& ctx) noexcept
    {
        std::uint32_t seq = 0;

        while (true) {
            const auto half = static_cast<esp_cpu_cycle_count_t>(ctx.half.read());

            Led::template write<true>();
            emit(ctx, seq, 1U);
            arc::fence();
            arc::Clock::spin(half);

            Led::template write<false>();
            emit(ctx, seq, 0U);
            arc::fence();
            arc::Clock::spin(half);
        }
    }

private:
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t mask(
        const std::uint8_t stride) noexcept
    {
        const auto shift = stride >= 31U ? 31U : stride;
        return shift == 0U ? 0U : ((std::uint32_t{1} << shift) - 1U);
    }

    IRAM_ATTR [[gnu::noinline]] static void emit(
        Ctx& ctx,
        std::uint32_t& seq,
        const std::uint8_t level) noexcept
    {
        const auto ctl = ctx.ctl.read();
        const auto edge = seq++;
        if ((ctl.flags & tx_on) == 0U) {
            return;
        }
        if ((edge & mask(ctl.stride)) != 0U) {
            return;
        }

        static_cast<void>(ctx.tx.try_push(Telemetry{
            .tick = static_cast<std::uint32_t>(arc::Clock::tick()),
            .seq = edge,
            .level = level,
            .mark = ctl.mark,
        }));
    }
};

using Rt = arc::Plane<Loop, cfg::stack, Ctx>;

constinit inline Ctx ctx{};

inline void boot()
{
    ctx.half.write(half_cycles(cfg::half_us));
    ctx.ctl.write(slow_ctl);
    net::boot(ctx);
    Rt::boot(net::tag, ctx);
}

}  // namespace app
