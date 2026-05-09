#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dsp.hpp"
#include "arc/result.hpp"

#if __has_include("psa/crypto.h") && __has_include("sha/sha_core.h")
#include "arc/sha.hpp"
#define ARC_PUF_SHA_AVAILABLE 1
#else
#define ARC_PUF_SHA_AVAILABLE 0
#endif

namespace arc::crypto {

struct PufStats {
    std::size_t raw_bits{};
    std::size_t stable_bits{};
    std::uint32_t ones{};
};

struct Puf {
    [[nodiscard]] static Result<std::size_t> sample_sram_decay(
        const std::span<const std::byte> sram,
        const std::span<std::uint8_t> raw) noexcept
    {
        if (sram.empty() || raw.size() < sram.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (std::size_t i = 0U; i < sram.size(); ++i) {
            raw[i] = std::to_integer<std::uint8_t>(sram[i]);
        }
        return sram.size();
    }

    template <typename Adc>
    [[nodiscard]] static Result<std::size_t> sample_adc_noise(
        const std::span<std::uint16_t> out,
        typename dsp::Biquad<std::int32_t>::State& filter,
        const typename dsp::Biquad<std::int32_t>::Coeffs coeffs = {.b0 = 1}) noexcept
    {
        if (out.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        for (std::size_t i = 0U; i < out.size(); ++i) {
            auto raw = Adc::read();
            if (!raw) {
                return fail(raw.error());
            }
            const auto filtered = dsp::Biquad<std::int32_t>::step(filter, coeffs, static_cast<std::int32_t>(*raw));
            out[i] = static_cast<std::uint16_t>(filtered < 0 ? -filtered : filtered);
        }
        return out.size();
    }

    [[nodiscard]] static PufStats von_neumann(
        const std::span<const std::uint8_t> raw,
        const std::span<std::uint8_t> stable) noexcept
    {
        for (auto& byte : stable) {
            byte = 0U;
        }

        PufStats stats{.raw_bits = raw.size() * 8U};
        auto out_bit = std::size_t{};
        for (std::size_t bit = 0U; bit + 1U < stats.raw_bits; bit += 2U) {
            const auto a = bit_at(raw, bit);
            const auto b = bit_at(raw, bit + 1U);
            if (a == b) {
                continue;
            }
            if ((out_bit / 8U) >= stable.size()) {
                break;
            }
            const auto value = a ? 1U : 0U;
            stable[out_bit / 8U] |= static_cast<std::uint8_t>(value << (out_bit & 7U));
            stats.ones += value;
            ++out_bit;
        }
        stats.stable_bits = out_bit;
        return stats;
    }

#if ARC_PUF_SHA_AVAILABLE
    [[nodiscard]] static Result<std::array<std::uint8_t, 32>> derive_key(
        const std::span<const std::uint8_t> stable) noexcept
    {
        if (stable.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return Sha::template sum<SHA2_256>(stable);
    }
#endif

    template <typename Hash>
    [[nodiscard]] static Result<std::array<std::uint8_t, 32>> derive_key_with(
        const std::span<const std::uint8_t> stable) noexcept
    {
        if (stable.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return Hash::sha256(stable);
    }

private:
    [[nodiscard]] static bool bit_at(
        const std::span<const std::uint8_t> bytes,
        const std::size_t bit) noexcept
    {
        return ((bytes[bit / 8U] >> (bit & 7U)) & 1U) != 0U;
    }
};

}  // namespace arc::crypto

#undef ARC_PUF_SHA_AVAILABLE
