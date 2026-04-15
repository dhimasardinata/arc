#pragma once

#include <cstdint>

#include "arc.hpp"
#include "cfg.hpp"

namespace app {

inline constexpr char tag[] = "arc-udp";
inline constexpr std::uint8_t trace_on = 1U << 0;

struct Edge {
    std::uint32_t tick;
    std::uint32_t seq;
    std::uint8_t level;
    std::uint8_t mark;
    std::uint16_t pad{};
};

static_assert(sizeof(Edge) == 12U);

struct Control {
    std::uint8_t mark;
    std::uint8_t stride;
    std::uint8_t flags;
    std::uint8_t spare{};
};

static_assert(sizeof(Control) == sizeof(std::uint32_t));

[[nodiscard]] constexpr std::uint32_t cycles(const std::uint32_t half_us) noexcept
{
    return static_cast<std::uint32_t>(arc::Clock::us(half_us, cfg::mhz));
}

inline constexpr Control slow{.mark = 0x31, .stride = 0, .flags = trace_on};
inline constexpr Control fast{.mark = 0x7a, .stride = 3, .flags = trace_on};

template <typename Bus>
inline void boot(Bus& bus) noexcept
{
    bus.pace.write(cycles(cfg::half_us));
    bus.control.write(slow);
}

}  // namespace app
