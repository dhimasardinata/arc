#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc {

struct SdioSlaveConfig {
    std::uint16_t block_bytes{512U};
    std::uint8_t queue_depth{4U};
    bool host_interrupts{true};
};

struct SdioSlaveTransfer {
    std::uintptr_t address{};
    std::size_t bytes{};
    std::uint8_t function{1U};
};

struct SdioSlave {
    template <typename Policy>
    [[nodiscard]] static Status start(const SdioSlaveConfig config = {}) noexcept
    {
        if (config.block_bytes == 0U || config.queue_depth == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::sdio_start(config));
    }

    template <typename Policy>
    [[nodiscard]] static Status queue_coherent(const std::span<std::uint8_t> buffer) noexcept
    {
        if (buffer.empty()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::sdio_queue(buffer));
    }

    template <typename Policy>
    [[nodiscard]] static Result<SdioSlaveTransfer> finish_coherent(const std::uint32_t timeout_ms = 0U) noexcept
    {
        return Policy::sdio_finish(timeout_ms);
    }

    template <typename Policy>
    [[nodiscard]] static Status interrupt_host(const std::uint32_t bits) noexcept
    {
        if (bits == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::sdio_interrupt(bits));
    }
};

}  // namespace arc
