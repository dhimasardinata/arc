#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/blackbox.hpp"
#include "arc/cloak.hpp"
#include "arc/kyber.hpp"
#include "arc/puf.hpp"

namespace arc::crypto {

struct SelfTestReport {
    std::uint32_t checks{};
    std::uint32_t failures{};

    [[nodiscard]] constexpr bool pass() const noexcept
    {
        return failures == 0U;
    }

    constexpr void note(const bool ok) noexcept
    {
        ++checks;
        failures += ok ? 0U : 1U;
    }

    constexpr void merge(const SelfTestReport next) noexcept
    {
        checks += next.checks;
        failures += next.failures;
    }
};

template <typename Kem = Kyber512>
struct KyberSelfTest {
    typename Kem::Poly poly{};
    typename Kem::Poly original{};
    std::array<std::uint8_t, Kem::public_key_bytes> public_key{};
    std::array<std::uint8_t, Kem::secret_key_bytes> secret_key{};
    std::array<std::uint8_t, Kem::ciphertext_bytes> ciphertext{};
    std::array<std::uint8_t, Kem::shared_bytes> shared{};
    std::array<std::uint8_t, Kem::shared_bytes> opened{};
};

template <std::size_t DigestBytes = 32U>
struct BlackBoxSelfTest {
    std::array<std::uint8_t, DigestBytes> previous{};
    std::array<std::uint8_t, 4> key{1U, 2U, 3U, 4U};
    std::array<std::uint8_t, 4> payload{5U, 6U, 7U, 8U};
};

struct SelfTest {
    template <typename Kem>
    [[nodiscard]] static SelfTestReport kyber(KyberSelfTest<Kem>& work) noexcept
    {
        SelfTestReport report{};
        for (std::size_t i = 0U; i < work.poly.size(); ++i) {
            work.poly[i] = static_cast<std::int16_t>((i * 3U) & 0x7fU);
        }
        work.original = work.poly;
        report.note(Kem::ntt(std::span<std::int16_t, Kem::n>{work.poly}).has_value());
        report.note(Kem::inverse_ntt(std::span<std::int16_t, Kem::n>{work.poly}).has_value());
        report.note(work.poly == work.original);

        constexpr std::array<std::uint8_t, 4> seed{1U, 2U, 3U, 4U};
        constexpr std::array<std::uint8_t, 4> coins{9U, 10U, 11U, 12U};
        report.note(Kem::keypair(seed, work.public_key, work.secret_key).has_value());
        report.note(Kem::encapsulate(work.public_key, coins, work.ciphertext, work.shared).has_value());
        report.note(Kem::decapsulate(work.secret_key, work.ciphertext, work.opened).has_value());
        report.note(work.shared == work.opened);
        report.note(work.ciphertext[0] != work.ciphertext[1] && work.shared[0] != 0U);
        return report;
    }

    [[nodiscard]] static SelfTestReport puf() noexcept
    {
        SelfTestReport report{};
        constexpr std::array<std::uint8_t, 1> raw{0b0001'1011U};
        std::array<std::uint8_t, 1> stable{};
        const auto stats = Puf::von_neumann(raw, stable);
        report.note(stats.raw_bits == 8U);
        report.note(stats.stable_bits == 2U);
        report.note(stats.ones == 1U);
        report.note(stable[0] == 0b10U);
        return report;
    }

    template <typename CloakPolicy>
    [[nodiscard]] static SelfTestReport cloak(const std::span<const std::byte> dummy) noexcept
    {
        SelfTestReport report{};
        const auto scrambled = Cloak::scramble<CloakPolicy>({.stall_mask = 0x03U, .dummy_reads = 3U}, dummy);
        const auto flattened = Cloak::flatten<CloakPolicy>({.flatten_ticks = 3U}, 1U);
        report.note(scrambled.stalls != 0U);
        report.note(dummy.empty() || scrambled.dummy_reads == 3U);
        report.note(flattened.flatten_heavy + flattened.flatten_light == 3U);
        return report;
    }

    template <typename HashPolicy, std::size_t DigestBytes>
    [[nodiscard]] static SelfTestReport blackbox(BlackBoxSelfTest<DigestBytes>& work) noexcept
    {
        SelfTestReport report{};
        const auto node = ::arc::covert::BlackBox::template seal<HashPolicy, DigestBytes>(
            1U,
            std::span<const std::uint8_t>{work.payload},
            std::span<const std::uint8_t, DigestBytes>{work.previous},
            std::span<const std::uint8_t>{work.key});
        report.note(node.has_value());
        if (node) {
            report.note(node->seq == 1U);
            report.note(node->payload_bytes == work.payload.size());
            report.note(node->root[0] != 0U || node->root[1] != 0U);
        } else {
            report.note(false);
            report.note(false);
            report.note(false);
        }
        return report;
    }
};

}  // namespace arc::crypto
