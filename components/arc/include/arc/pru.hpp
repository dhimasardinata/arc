#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "arc/caps.hpp"
#include "arc/dma_chain.hpp"
#include "arc/result.hpp"

namespace arc {

enum class PruBackend : std::uint8_t {
    lcd_cam,
    i2s_parallel,
};

struct PruTiming {
    std::uint32_t hz{};
    std::uint8_t width{16U};
    PruBackend backend{PruBackend::lcd_cam};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return hz != 0U && (width == 8U || width == 16U);
    }
};

struct PruCursor {
    std::size_t read{};
    std::size_t write{};
    std::size_t guard{};
};

template <std::size_t Descriptors>
struct PruOut {
    static_assert(Descriptors > 0U, "PRU output needs at least one descriptor");

    [[nodiscard]] static Status bind(
        DmaChain<Descriptors>& chain,
        const std::span<std::uint16_t> states,
        const std::size_t words_per_desc,
        const bool circular = true) noexcept
    {
        if (states.empty() || words_per_desc == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto* const bytes = reinterpret_cast<std::uint8_t*>(states.data());
        const auto raw = std::span<std::uint8_t>{bytes, states.size_bytes()};
        auto offset_words = std::size_t{};
        for (std::size_t i = 0; i < Descriptors; ++i) {
            if (offset_words >= states.size()) {
                chain.desc[i] = {};
                continue;
            }

            const auto take_words = words_per_desc < states.size() - offset_words ? words_per_desc : states.size() - offset_words;
            const auto offset_bytes = offset_words * sizeof(std::uint16_t);
            chain.bind(i, raw.subspan(offset_bytes, take_words * sizeof(std::uint16_t)), i + 1U == Descriptors && !circular);
            offset_words += take_words;
        }

        if (circular) {
            chain.desc[Descriptors - 1U].next = chain.head();
        }
        return ok();
    }

    [[nodiscard]] static Status mutate_ahead(
        const std::span<std::uint16_t> ring,
        const PruCursor cursor,
        const std::span<const std::uint16_t> patch) noexcept
    {
        if (ring.empty() || patch.empty() || patch.size() + cursor.guard > ring.size()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto out = (cursor.read + cursor.guard) % ring.size();
        for (const auto value : patch) {
            ring[out] = value;
            out = (out + 1U) % ring.size();
        }
        return ok();
    }
};

template <std::size_t Descriptors>
struct PruIn {
    static_assert(Descriptors > 0U, "PRU input needs at least one descriptor");

    template <typename T, std::size_t Extent>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] static Status bind(
        DmaChain<Descriptors>& chain,
        const std::span<T, Extent> capture,
        const std::size_t samples_per_desc,
        const bool circular = true) noexcept
    {
        if (capture.empty() || samples_per_desc == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto* const bytes = reinterpret_cast<std::uint8_t*>(capture.data());
        const auto raw = std::span<std::uint8_t>{bytes, capture.size_bytes()};
        auto offset_samples = std::size_t{};
        for (std::size_t i = 0; i < Descriptors; ++i) {
            if (offset_samples >= capture.size()) {
                chain.desc[i] = {};
                continue;
            }

            const auto take_samples = samples_per_desc < capture.size() - offset_samples ? samples_per_desc : capture.size() - offset_samples;
            const auto offset_bytes = offset_samples * sizeof(T);
            chain.bind(i, raw.subspan(offset_bytes, take_samples * sizeof(T)), i + 1U == Descriptors && !circular);
            offset_samples += take_samples;
        }

        if (circular) {
            chain.desc[Descriptors - 1U].next = chain.head();
        }
        return ok();
    }

    template <typename T>
    [[nodiscard]] static CapsBuf<T> psram_buffer(const std::size_t samples) noexcept
    {
        return psbuf<T, arc::cache_line>(samples);
    }
};

}  // namespace arc
