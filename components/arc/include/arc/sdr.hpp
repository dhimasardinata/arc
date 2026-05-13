#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dma_chain.hpp"
#include "arc/dsp.hpp"
#include "arc/result.hpp"

namespace arc::sdr {

enum class Modulation : std::uint8_t {
    ook,
    am,
    fm,
};

struct TxConfig {
    std::uint32_t sample_hz{};
    std::uint32_t carrier_hz{};
    std::uint32_t deviation_hz{};
    std::uint32_t gpio{4U};
    std::uint16_t one_word{1U};
    std::uint16_t zero_word{};
};

struct TxStats {
    std::size_t words{};
    std::uint32_t carrier_hz{};
    std::uint32_t sample_hz{};
};

struct PulseSynth {
    [[nodiscard]] static constexpr bool valid(const TxConfig config) noexcept
    {
        return config.sample_hz != 0U &&
            config.carrier_hz != 0U &&
            config.carrier_hz < (config.sample_hz / 2U);
    }

    [[nodiscard]] static Result<TxStats> ook(
        const std::span<std::uint16_t> out,
        const std::span<const std::uint8_t> bytes,
        const TxConfig config,
        const bool msb_first = true) noexcept
    {
        if (!valid(config) || out.size() < bytes.size() * 8U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto pos = std::size_t{};
        for (const auto byte : bytes) {
            for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
                const auto shift = msb_first ? 7U - bit : bit;
                out[pos++] = ((byte >> shift) & 1U) != 0U ? config.one_word : config.zero_word;
            }
        }
        return TxStats{.words = pos, .carrier_hz = config.carrier_hz, .sample_hz = config.sample_hz};
    }

    [[nodiscard]] static Result<TxStats> fm(
        const std::span<std::uint16_t> out,
        const std::span<const std::int16_t> audio,
        const TxConfig config) noexcept
    {
        if (!valid(config) || config.deviation_hz == 0U || out.size() < audio.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto phase = std::uint32_t{};
        for (std::size_t i = 0U; i < audio.size(); ++i) {
            const auto signed_dev = (static_cast<std::int64_t>(audio[i]) * config.deviation_hz) / 32768;
            const auto hz = dsp::clamp<std::int64_t>(
                static_cast<std::int64_t>(config.carrier_hz) + signed_dev,
                1,
                static_cast<std::int64_t>((config.sample_hz / 2U) - 1U));
            phase = (phase + static_cast<std::uint32_t>(hz)) % config.sample_hz;
            out[i] = phase < (config.sample_hz / 2U) ? config.one_word : config.zero_word;
        }
        return TxStats{.words = audio.size(), .carrier_hz = config.carrier_hz, .sample_hz = config.sample_hz};
    }

    [[nodiscard]] static Result<TxStats> am(
        const std::span<std::uint16_t> out,
        const std::span<const std::uint16_t> envelope_permille,
        const TxConfig config) noexcept
    {
        if (!valid(config) || out.size() < envelope_permille.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto phase = std::uint32_t{};
        for (std::size_t i = 0U; i < envelope_permille.size(); ++i) {
            phase = (phase + config.carrier_hz) % config.sample_hz;
            const auto duty = dsp::clamp<std::uint32_t>(envelope_permille[i], 0U, 1000U);
            out[i] = ((phase * 1000U) / config.sample_hz) < duty ? config.one_word : config.zero_word;
        }
        return TxStats{.words = envelope_permille.size(), .carrier_hz = config.carrier_hz, .sample_hz = config.sample_hz};
    }
};

template <std::size_t Descriptors>
[[nodiscard]] Status bind_dma(
    DmaChain<Descriptors>& chain,
    const std::span<std::uint16_t> words,
    const bool circular = true) noexcept
{
    if (words.empty()) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    auto bytes = std::span<std::uint8_t>{
        reinterpret_cast<std::uint8_t*>(words.data()),
        words.size_bytes(),
    };
    chain.bind(0U, bytes, !circular);
    if (circular) {
        chain.link_circular();
    }
    return ok();
}

template <typename Policy>
struct Tx {
    [[nodiscard]] static Status start(
        const TxConfig config,
        const std::span<const std::uint16_t> words) noexcept
    {
        if (!PulseSynth::valid(config) || words.empty()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::cam_start(config, words));
    }

    [[nodiscard]] static Status stop() noexcept
    {
        if constexpr (requires { Policy::cam_stop(); }) {
            return status(Policy::cam_stop());
        } else {
            return ok();
        }
    }
};

}  // namespace arc::sdr
