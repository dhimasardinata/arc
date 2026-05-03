#pragma once

#include <cstdint>

namespace arc {

struct HilDeadline {
    std::uint32_t expected_tick{};
    std::uint32_t observed_tick{};
    std::uint32_t tolerance_ticks{};
};

struct Hil {
    [[nodiscard]] static constexpr bool within(const HilDeadline deadline) noexcept
    {
        const auto diff = deadline.observed_tick > deadline.expected_tick
            ? deadline.observed_tick - deadline.expected_tick
            : deadline.expected_tick - deadline.observed_tick;
        return diff <= deadline.tolerance_ticks;
    }
};

}  // namespace arc
