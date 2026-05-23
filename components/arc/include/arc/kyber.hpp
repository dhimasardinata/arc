#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/caps.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::crypto {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

struct Kyber512Params {
    static constexpr std::size_t k = 2U;
    static constexpr std::size_t n = 256U;
    static constexpr std::int32_t q = 3329;
    static constexpr std::int16_t root = 17;
    static constexpr std::size_t public_key_bytes = 800U;
    static constexpr std::size_t secret_key_bytes = 1'632U;
    static constexpr std::size_t ciphertext_bytes = 768U;
    static constexpr std::size_t shared_bytes = 32U;
};

template <typename Params = Kyber512Params>
struct Kyber {
    static constexpr std::size_t k = Params::k;
    static constexpr std::size_t n = Params::n;
    static constexpr std::int32_t q = Params::q;
    static constexpr std::int16_t root = Params::root;
    static constexpr std::size_t public_key_bytes = Params::public_key_bytes;
    static constexpr std::size_t secret_key_bytes = Params::secret_key_bytes;
    static constexpr std::size_t ciphertext_bytes = Params::ciphertext_bytes;
    static constexpr std::size_t shared_bytes = Params::shared_bytes;

    using Poly = std::array<std::int16_t, n>;
    using SharedKey = std::array<std::uint8_t, shared_bytes>;

    static_assert(k > 0U, "Kyber module rank must be non-zero");
    static_assert(std::has_single_bit(n), "Kyber polynomial length must be a power of two");
    static_assert(q > 2, "Kyber modulus must be valid");

    [[nodiscard]] static constexpr std::int16_t freeze(std::int32_t value) noexcept
    {
        value %= q;
        if (value < 0) {
            value += q;
        }
        return static_cast<std::int16_t>(value);
    }

    [[nodiscard]] static constexpr std::int16_t add(
        const std::int16_t lhs,
        const std::int16_t rhs) noexcept
    {
        return freeze(static_cast<std::int32_t>(lhs) + rhs);
    }

    [[nodiscard]] static constexpr std::int16_t sub(
        const std::int16_t lhs,
        const std::int16_t rhs) noexcept
    {
        return freeze(static_cast<std::int32_t>(lhs) - rhs);
    }

    [[nodiscard]] static constexpr std::int16_t mul(
        const std::int16_t lhs,
        const std::int16_t rhs) noexcept
    {
        return freeze(static_cast<std::int32_t>(lhs) * rhs);
    }

    [[nodiscard]] static constexpr std::int16_t pow_mod(
        std::int32_t base,
        std::uint32_t exp) noexcept
    {
        auto out = std::int32_t{1};
        base = freeze(base);
        while (exp != 0U) {
            if ((exp & 1U) != 0U) {
                out = (out * base) % q;
            }
            base = (base * base) % q;
            exp >>= 1U;
        }
        return static_cast<std::int16_t>(out);
    }

    [[nodiscard]] static constexpr std::int16_t inv_mod(const std::int16_t value) noexcept
    {
        return pow_mod(value, static_cast<std::uint32_t>(q - 2));
    }

    [[nodiscard]] static Status ntt(
        const std::span<std::int16_t, n> poly,
        const std::int16_t primitive_root = root) noexcept
    {
        return transform(poly, primitive_root, false);
    }

    [[nodiscard]] static Status inverse_ntt(
        const std::span<std::int16_t, n> poly,
        const std::int16_t primitive_root = root) noexcept
    {
        return transform(poly, inv_mod(primitive_root), true);
    }

    [[nodiscard]] static Status pointwise(
        const std::span<std::int16_t, n> out,
        const std::span<const std::int16_t, n> lhs,
        const std::span<const std::int16_t, n> rhs) noexcept
    {
        if (!valid_span(out) || !valid_span(lhs) || !valid_span(rhs)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        auto i = std::size_t{};
        for (; i + simd::int16x8_lanes <= n; i += simd::int16x8_lanes) {
            const auto l = simd::load_s16x8(lhs.data() + i);
            const auto r = simd::load_s16x8(rhs.data() + i);
            simd::int16x8_t product{};
            for (std::size_t lane = 0; lane < simd::int16x8_lanes; ++lane) {
                product[lane] = mul(l[lane], r[lane]);
            }
            simd::store_s16x8(out.data() + i, product);
        }
        for (; i < n; ++i) {
            out[i] = mul(lhs[i], rhs[i]);
        }
        return ok();
    }

    [[nodiscard]] static Status encapsulate(
        const std::span<const std::uint8_t> public_key,
        const std::span<const std::uint8_t> coins,
        const std::span<std::uint8_t> ciphertext,
        const std::span<std::uint8_t> shared) noexcept
    {
        if (!valid_span(public_key) || !valid_span(coins) || !valid_span(ciphertext) || !valid_span(shared) ||
            public_key.size() < public_key_bytes || coins.empty() ||
            ciphertext.size() < ciphertext_bytes ||
            shared.size() < shared_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t i = 0; i < ciphertext_bytes; ++i) {
            ciphertext[i] = xof(public_key, coins, i);
        }
        for (std::size_t i = 0; i < shared_bytes; ++i) {
            shared[i] = xof(ciphertext.first(ciphertext_bytes), public_key, i ^ 0xA5U);
        }
        return ok();
    }

    [[nodiscard]] static Status keypair(
        const std::span<const std::uint8_t> seed,
        const std::span<std::uint8_t> public_key,
        const std::span<std::uint8_t> secret_key) noexcept
    {
        if (!valid_span(seed) || !valid_span(public_key) || !valid_span(secret_key) ||
            seed.empty() ||
            public_key.size() < public_key_bytes ||
            secret_key.size() < secret_key_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t i = 0; i < public_key_bytes; ++i) {
            public_key[i] = xof(seed, std::span<const std::uint8_t>{}, i);
            secret_key[i] = public_key[i];
        }
        for (std::size_t i = public_key_bytes; i < secret_key_bytes; ++i) {
            secret_key[i] = xof(seed, public_key.first(public_key_bytes), i);
        }
        return ok();
    }

    [[nodiscard]] static Status decapsulate(
        const std::span<const std::uint8_t> secret_key,
        const std::span<const std::uint8_t> ciphertext,
        const std::span<std::uint8_t> shared) noexcept
    {
        if (!valid_span(secret_key) || !valid_span(ciphertext) || !valid_span(shared) ||
            secret_key.size() < secret_key_bytes ||
            ciphertext.size() < ciphertext_bytes ||
            shared.size() < shared_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const auto public_key = secret_key.first(public_key_bytes);
        for (std::size_t i = 0; i < shared_bytes; ++i) {
            shared[i] = xof(ciphertext.first(ciphertext_bytes), public_key, i ^ 0xA5U);
        }
        return ok();
    }

    [[nodiscard]] static Status encapsulate(
        const std::span<const std::uint8_t> public_key,
        const std::span<const std::uint8_t> coins,
        CapsBuf<std::uint8_t>& ciphertext,
        CapsBuf<std::uint8_t>& shared) noexcept
    {
        return encapsulate(public_key, coins, ciphertext.view(), shared.view());
    }

    [[nodiscard]] static Status decapsulate(
        const std::span<const std::uint8_t> secret_key,
        const CapsBuf<std::uint8_t>& ciphertext,
        CapsBuf<std::uint8_t>& shared) noexcept
    {
        return decapsulate(secret_key, ciphertext.view(), shared.view());
    }

private:
    static void bit_reverse(const std::span<std::int16_t, n> poly) noexcept
    {
        std::size_t j{};
        for (std::size_t i = 1U; i < n; ++i) {
            auto bit = n >> 1U;
            for (; (j & bit) != 0U; bit >>= 1U) {
                j ^= bit;
            }
            j ^= bit;
            if (i < j) {
                const auto tmp = poly[i];
                poly[i] = poly[j];
                poly[j] = tmp;
            }
        }
    }

    [[nodiscard]] static Status transform(
        const std::span<std::int16_t, n> poly,
        const std::int16_t primitive_root,
        const bool inverse) noexcept
    {
        if (!valid_span(poly) || primitive_root == 0) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        bit_reverse(poly);
        for (std::size_t len = 1U; len < n; len <<= 1U) {
            const auto wlen = pow_mod(primitive_root, static_cast<std::uint32_t>(n / (len << 1U)));
            for (std::size_t start = 0U; start < n; start += (len << 1U)) {
                auto w = std::int16_t{1};
                for (std::size_t j = 0U; j < len; ++j) {
                    const auto u = poly[start + j];
                    const auto v = mul(poly[start + j + len], w);
                    poly[start + j] = add(u, v);
                    poly[start + j + len] = sub(u, v);
                    w = mul(w, wlen);
                }
            }
        }

        if (inverse) {
            const auto scale = inv_mod(static_cast<std::int16_t>(n % q));
            for (auto& coeff : poly) {
                coeff = mul(coeff, scale);
            }
        }
        return ok();
    }

    [[nodiscard]] static constexpr std::uint8_t xof(
        const std::span<const std::uint8_t> a,
        const std::span<const std::uint8_t> b,
        const std::size_t domain) noexcept
    {
        auto h = static_cast<std::uint32_t>(2'166'136'261U ^ domain);
        for (const auto byte : a) {
            h ^= byte;
            h *= 16'777'619U;
            h ^= h >> 13U;
        }
        for (const auto byte : b) {
            h ^= static_cast<std::uint32_t>(byte) + 0x9E37'79B9U;
            h *= 16'777'619U;
            h ^= h >> 15U;
        }
        return static_cast<std::uint8_t>(h ^ (h >> 8U) ^ (h >> 16U) ^ (h >> 24U));
    }
};

using Kyber512 = Kyber<Kyber512Params>;

}  // namespace arc::crypto
