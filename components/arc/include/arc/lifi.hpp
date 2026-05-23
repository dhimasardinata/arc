#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::optical {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool lifi_valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

}  // namespace detail

struct LiFiConfig {
    std::uint16_t half_ticks{1U};
    bool idle_high{};
};

struct LiFiSymbol {
    std::uint16_t first_ticks{};
    bool first_high{};
    std::uint16_t second_ticks{};
    bool second_high{};
};

struct LiFi {
    [[nodiscard]] static constexpr bool valid(const LiFiConfig config) noexcept
    {
        return config.half_ticks != 0U;
    }

    [[nodiscard]] static constexpr LiFiSymbol bit(
        const bool one,
        const LiFiConfig config = {}) noexcept
    {
        return {
            .first_ticks = config.half_ticks,
            .first_high = one,
            .second_ticks = config.half_ticks,
            .second_high = !one,
        };
    }

    [[nodiscard]] static constexpr LiFiSymbol idle(const LiFiConfig config = {}) noexcept
    {
        return {
            .first_ticks = config.half_ticks,
            .first_high = config.idle_high,
            .second_ticks = config.half_ticks,
            .second_high = config.idle_high,
        };
    }

    [[nodiscard]] static constexpr bool fits(const std::size_t bytes, const std::size_t symbols) noexcept
    {
        return bytes <= std::numeric_limits<std::size_t>::max() / 8U && symbols >= bytes * 8U;
    }

    [[nodiscard]] static Result<std::span<LiFiSymbol>> encode(
        const std::span<const std::uint8_t> bytes,
        const std::span<LiFiSymbol> out,
        const LiFiConfig config = {},
        const bool msb_first = true) noexcept
    {
        if (!valid(config) || !detail::lifi_valid_span(bytes) || !detail::lifi_valid_span(out) ||
            !fits(bytes.size(), out.size())) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        auto pos = std::size_t{};
        for (const auto byte : bytes) {
            for (std::uint8_t i = 0U; i < 8U; ++i) {
                const auto shift = msb_first ? 7U - i : i;
                out[pos++] = bit(((byte >> shift) & 1U) != 0U, config);
            }
        }
        return out.first(pos);
    }

    template <typename Policy>
    [[nodiscard]] static Status transmit(const std::span<const LiFiSymbol> symbols) noexcept
    {
        if (symbols.empty() || !detail::lifi_valid_span(symbols)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::send(symbols));
    }

    template <typename Policy>
    [[nodiscard]] static Status arm_receiver() noexcept
    {
        return status(Policy::arm_receiver());
    }
};

}  // namespace arc::optical
