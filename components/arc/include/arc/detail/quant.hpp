#pragma once

#include <cstdint>
#include <limits>

namespace arc::detail {

[[nodiscard]] constexpr std::int64_t mul_sat(
    const std::int64_t lhs,
    const std::int32_t rhs) noexcept
{
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    const auto rhs64 = static_cast<std::int64_t>(rhs);

    if (lhs == 0 || rhs == 0) {
        return 0;
    }
    if (lhs > 0) {
        if (rhs64 > 0 && lhs > max / rhs64) {
            return max;
        }
        if (rhs64 < 0 && rhs64 < min / lhs) {
            return min;
        }
    } else {
        if (rhs64 > 0 && lhs < min / rhs64) {
            return min;
        }
        if (rhs64 < 0 && lhs < max / rhs64) {
            return max;
        }
    }
    return lhs * rhs64;
}

[[nodiscard]] constexpr std::int64_t add_sat(
    const std::int64_t lhs,
    const std::int32_t rhs) noexcept
{
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    constexpr auto min = std::numeric_limits<std::int64_t>::min();
    const auto rhs64 = static_cast<std::int64_t>(rhs);

    if (rhs64 > 0 && lhs > max - rhs64) {
        return max;
    }
    if (rhs64 < 0 && lhs < min - rhs64) {
        return min;
    }
    return lhs + rhs64;
}

[[nodiscard]] constexpr std::int64_t round_shift_s64(
    const std::int64_t value,
    const std::uint8_t shift) noexcept
{
    if (shift == 0U) {
        return value;
    }
    if (shift > 63U) {
        return 0;
    }

    const auto magnitude = value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1U : static_cast<std::uint64_t>(value);
    const auto rounded = (magnitude + (std::uint64_t{1} << (shift - 1U))) >> shift;
    return value < 0 ? -static_cast<std::int64_t>(rounded) : static_cast<std::int64_t>(rounded);
}

}  // namespace arc::detail
