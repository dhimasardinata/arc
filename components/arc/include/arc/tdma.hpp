#pragma once

#include <cstdint>

namespace arc::net {

struct TdmaConfig {
    std::uint32_t period_us{};
    std::uint32_t slot_us{};
    std::uint32_t guard_us{};
    std::uint32_t slot{};
};

struct TdmaWindow {
    std::uint32_t start_us{};
    std::uint32_t end_us{};
    bool active{};
};

struct Tdma {
    [[nodiscard]] static constexpr TdmaWindow window(
        const std::uint64_t now_us,
        const TdmaConfig config) noexcept
    {
        if (config.period_us == 0U || config.slot_us == 0U || config.guard_us >= config.slot_us) {
            return {};
        }

        const auto slot_start = config.slot * config.slot_us;
        if (slot_start >= config.period_us) {
            return {};
        }

        const auto phase = static_cast<std::uint32_t>(now_us % config.period_us);
        const auto start = slot_start + config.guard_us;
        const auto end = slot_start + config.slot_us - config.guard_us;
        return {
            .start_us = start,
            .end_us = end,
            .active = phase >= start && phase < end,
        };
    }
};

}  // namespace arc::net
