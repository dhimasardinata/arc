#pragma once

#include <cstdint>

#include "arc/clock.hpp"
#include "arc/reg.hpp"
#include "arc/ring.hpp"
#include "cfg.hpp"

namespace app {

inline constexpr std::uint8_t tx_on = 1U << 0;

struct Telemetry {
    std::uint32_t tick;
    std::uint32_t seq;
    std::uint8_t level;
    std::uint8_t mark;
    std::uint16_t pad{};
};

static_assert(sizeof(Telemetry) == 12U);

struct Ctl {
    std::uint8_t mark;
    std::uint8_t stride;
    std::uint8_t flags;
    std::uint8_t spare{};
};

static_assert(sizeof(Ctl) == sizeof(std::uint32_t));

struct Ctx {
    arc::Ring<Telemetry, 256> tx{};
    arc::Reg<std::uint32_t> half{};
    arc::Reg<Ctl> ctl{};
};

[[nodiscard]] constexpr std::uint32_t half_cycles(const std::uint32_t half_us) noexcept
{
    return static_cast<std::uint32_t>(arc::Clock::us(half_us, cfg::mhz));
}

inline constexpr Ctl slow_ctl{.mark = 0x31, .stride = 0, .flags = tx_on};
inline constexpr Ctl fast_ctl{.mark = 0x7a, .stride = 3, .flags = tx_on};

}  // namespace app
