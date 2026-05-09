#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::trace {

struct LiveStreamConfig {
    std::size_t bank_bytes{32U * 1024U};
    std::size_t watermark_bytes{16U * 1024U};
    bool swap_banks{true};
};

struct LiveChunk {
    std::span<const std::uint8_t> bytes{};
    std::uint8_t bank{};
    bool swapped{};
};

struct LiveStream {
    template <typename Policy>
    [[nodiscard]] static Status arm(const LiveStreamConfig config = {}) noexcept
    {
        if (config.bank_bytes == 0U || config.watermark_bytes == 0U || config.watermark_bytes > config.bank_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::trace_arm(config));
    }

    template <typename Policy>
    [[nodiscard]] static Result<LiveChunk> half_full_isr(const LiveStreamConfig config = {}) noexcept
    {
        auto chunk = Policy::trace_half_full(config.watermark_bytes);
        if (!chunk) {
            return fail(chunk.error());
        }
        if (chunk->bytes.empty()) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        if (config.swap_banks) {
            if (const auto err = Policy::trace_swap(); err != ESP_OK) {
                return fail(err);
            }
            chunk->swapped = true;
        }
        return *chunk;
    }

    template <typename Sink>
    [[nodiscard]] static Status exfiltrate(const LiveChunk chunk, Sink& sink) noexcept
    {
        if (chunk.bytes.empty()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(sink.write(chunk.bytes));
    }

    template <typename Policy, typename Sink>
    [[nodiscard]] static Status drain_half_full_isr(
        Sink& sink,
        const LiveStreamConfig config = {}) noexcept
    {
        auto chunk = half_full_isr<Policy>(config);
        if (!chunk) {
            return Status{fail(chunk.error())};
        }
        return exfiltrate(*chunk, sink);
    }
};

}  // namespace arc::trace
