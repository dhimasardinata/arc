#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::covert {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool covert_valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

}  // namespace detail

struct FskConfig {
    std::uint32_t mark_hz{};
    std::uint32_t space_hz{};
    std::uint32_t symbol_us{1000U};
    std::uint32_t duty_permille{500U};
    std::int8_t mark_density{96};
    std::int8_t space_density{-96};
};

struct FskSymbol {
    std::uint32_t hz{};
    std::uint32_t duration_us{};
    std::uint32_t duty_permille{};
    std::int8_t density{};
    bool mark{};
};

struct Fsk {
    [[nodiscard]] static constexpr bool valid(const FskConfig config) noexcept
    {
        return config.mark_hz != 0U &&
            config.space_hz != 0U &&
            config.mark_hz != config.space_hz &&
            config.symbol_us != 0U &&
            config.duty_permille <= 1000U;
    }

    [[nodiscard]] static constexpr FskSymbol symbol(
        const bool mark,
        const FskConfig config) noexcept
    {
        return {
            .hz = mark ? config.mark_hz : config.space_hz,
            .duration_us = config.symbol_us,
            .duty_permille = config.duty_permille,
            .density = mark ? config.mark_density : config.space_density,
            .mark = mark,
        };
    }

    [[nodiscard]] static constexpr bool fits(const std::size_t bytes, const std::size_t symbols) noexcept
    {
        return bytes <= std::numeric_limits<std::size_t>::max() / 8U && symbols >= bytes * 8U;
    }

    template <std::size_t Symbols>
    [[nodiscard]] static Status plan(
        const std::span<FskSymbol, Symbols> out,
        const std::span<const std::uint8_t> bytes,
        const FskConfig config,
        const bool msb_first = true) noexcept
    {
        if (!valid(config) || !detail::covert_valid_span(out) || !detail::covert_valid_span(bytes) ||
            !fits(bytes.size(), out.size())) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto pos = std::size_t{};
        for (const auto byte : bytes) {
            for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
                const auto shift = msb_first ? 7U - bit : bit;
                out[pos++] = symbol(((byte >> shift) & 1U) != 0U, config);
            }
        }
        return ok();
    }
};

template <typename Wave>
struct EmTx {
    [[nodiscard]] static Status bit(
        const bool mark,
        const FskConfig config) noexcept
    {
        if (!Fsk::valid(config)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        return emit(Fsk::symbol(mark, config));
    }

    template <std::size_t Symbols>
    [[nodiscard]] static Status symbols(const std::span<const FskSymbol, Symbols> symbols) noexcept
    {
        if (!detail::covert_valid_span(symbols)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        for (const auto sym : symbols) {
            auto sent = emit(sym);
            if (!sent) {
                return sent;
            }
        }
        return ok();
    }

private:
    [[nodiscard]] static Status emit(const FskSymbol sym) noexcept
    {
        if (sym.hz == 0U || sym.duration_us == 0U || sym.duty_permille > 1000U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if constexpr (requires { Wave::hz(sym.hz); Wave::duty(sym.duty_permille); }) {
            if (const auto err = Wave::hz(sym.hz); err != ESP_OK) {
                return Status{fail(err)};
            }
            return status(Wave::duty(sym.duty_permille));
        } else if constexpr (requires { Wave::set(sym.density); }) {
            return status(Wave::set(sym.density));
        } else {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
    }
};

template <typename Wave>
struct SonicTx {
    [[nodiscard]] static Status tone(
        const std::uint32_t hz,
        const std::uint32_t duty_permille = 500U) noexcept
    {
        if (hz == 0U || duty_permille > 1000U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if constexpr (requires { Wave::hz(hz); Wave::duty(duty_permille); }) {
            if (const auto err = Wave::hz(hz); err != ESP_OK) {
                return Status{fail(err)};
            }
            return status(Wave::duty(duty_permille));
        } else {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
    }

    [[nodiscard]] static Status bit(
        const bool mark,
        const FskConfig config) noexcept
    {
        if (!Fsk::valid(config)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return tone(mark ? config.mark_hz : config.space_hz, config.duty_permille);
    }
};

}  // namespace arc::covert
