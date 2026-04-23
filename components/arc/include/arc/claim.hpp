#pragma once

#include <cstdint>

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
    touch_bus = 0x70c0'0001U,
    touch_chan = 0x70c0'0002U,
};

[[nodiscard]] consteval std::uint32_t claim_mix(
    const std::uint32_t seed,
    const std::uint32_t value) noexcept
{
    return (seed ^ value) * 16'777'619U;
}

template <auto... Values>
[[nodiscard]] consteval std::uint32_t claim_token() noexcept
{
    std::uint32_t out = 2'166'136'261U;
    ((out = claim_mix(out, static_cast<std::uint32_t>(Values))), ...);
    return out == 0U ? 1U : out;
}

template <ClaimKind Kind, int Index>
struct ClaimSlot {
    alignas(4) constinit static inline std::uint32_t token{};
};

template <ClaimKind Kind, int Index, std::uint32_t Token>
struct Claim {
    static_assert(Token != 0U, "Arc claim token must be non-zero");

    [[nodiscard]] static esp_err_t take() noexcept
    {
        auto expected = std::uint32_t{};
        if (__atomic_compare_exchange_n(
                &ClaimSlot<Kind, Index>::token,
                &expected,
                Token,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return ESP_OK;
        }

        return expected == Token ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    static void drop() noexcept
    {
        auto expected = Token;
        static_cast<void>(__atomic_compare_exchange_n(
            &ClaimSlot<Kind, Index>::token,
            &expected,
            0U,
            false,
            __ATOMIC_ACQ_REL,
            __ATOMIC_ACQUIRE));
    }

    [[nodiscard]] static bool held() noexcept
    {
        return __atomic_load_n(&ClaimSlot<Kind, Index>::token, __ATOMIC_ACQUIRE) == Token;
    }
};

}  // namespace arc
