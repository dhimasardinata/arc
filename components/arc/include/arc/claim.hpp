#pragma once

#include <cstdint>

#include "arc/fence.hpp"
#include "esp_err.h"

namespace arc {

enum class ClaimKind : std::uint32_t {
    i2c_bus = 0x12c0'0001U,
    i2c_dev = 0x12c0'0002U,
    spi_bus = 0x15c0'0001U,
    spi_dev = 0x15c0'0002U,
    uart = 0x0a47'0001U,
    adc_bus = 0xadc0'0001U,
    adc_dev = 0xadc0'0002U,
    lcd_rgb = 0x1cd0'0001U,
    touch_bus = 0x70c0'0001U,
    touch_chan = 0x70c0'0002U,
    rtc_gpio = 0x27c0'0001U,
};

[[nodiscard]] consteval std::uint64_t claim_mix(
    const std::uint64_t seed,
    const std::uint64_t value) noexcept
{
    return (seed ^ value) * 1'099'511'628'211ULL;
}

template <auto... Values>
[[nodiscard]] consteval std::uint64_t claim_token() noexcept
{
    std::uint64_t out = 14'695'981'039'346'656'037ULL;
    ((out = claim_mix(out, static_cast<std::uint64_t>(Values))), ...);
    return out == 0ULL ? 1ULL : out;
}

template <ClaimKind Kind, int Index>
struct ClaimSlot {
    alignas(4) constinit static inline std::uint32_t gate{};
    alignas(8) constinit static inline std::uint64_t token{};
};

namespace detail {

IRAM_ATTR [[gnu::always_inline]] inline void claim_lock(std::uint32_t& gate) noexcept
{
    for (;;) {
        auto expected = std::uint32_t{};
        if (__atomic_compare_exchange_n(
                &gate,
                &expected,
                1U,
                false,
                __ATOMIC_ACQUIRE,
                __ATOMIC_RELAXED)) {
            return;
        }
        pause();
    }
}

IRAM_ATTR [[gnu::always_inline]] inline void claim_unlock(std::uint32_t& gate) noexcept
{
    __atomic_store_n(&gate, 0U, __ATOMIC_RELEASE);
}

}  // namespace detail

template <ClaimKind Kind, int Index, std::uint64_t Token>
struct Claim {
    static_assert(Token != 0ULL, "Arc claim token must be non-zero");

    [[nodiscard]] static esp_err_t take() noexcept
    {
        auto& gate = ClaimSlot<Kind, Index>::gate;
        auto& token = ClaimSlot<Kind, Index>::token;
        detail::claim_lock(gate);
        const auto current = token;
        if (current == 0ULL) {
            token = Token;
            detail::claim_unlock(gate);
            return ESP_OK;
        }
        detail::claim_unlock(gate);
        return current == Token ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    static void drop() noexcept
    {
        auto& gate = ClaimSlot<Kind, Index>::gate;
        auto& token = ClaimSlot<Kind, Index>::token;
        detail::claim_lock(gate);
        if (token == Token) {
            token = 0ULL;
        }
        detail::claim_unlock(gate);
    }

    [[nodiscard]] static bool held() noexcept
    {
        auto& gate = ClaimSlot<Kind, Index>::gate;
        auto& token = ClaimSlot<Kind, Index>::token;
        detail::claim_lock(gate);
        const auto ok = token == Token;
        detail::claim_unlock(gate);
        return ok;
    }
};

}  // namespace arc
