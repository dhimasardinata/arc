#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/cache.hpp"
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

inline constexpr std::uint32_t sdio_wait_forever = ~std::uint32_t{0};

template <typename Policy>
class SdioSlaveLease {
public:
    constexpr SdioSlaveLease() noexcept = default;

    explicit constexpr SdioSlaveLease(const std::span<std::uint8_t> buffer) noexcept
        : buffer_(buffer)
        , active_(true)
    {
    }

    SdioSlaveLease(const SdioSlaveLease&) = delete;
    SdioSlaveLease& operator=(const SdioSlaveLease&) = delete;

    constexpr SdioSlaveLease(SdioSlaveLease&& other) noexcept
        : buffer_(other.buffer_)
        , active_(other.active_)
    {
        other.buffer_ = {};
        other.active_ = false;
    }

    SdioSlaveLease& operator=(SdioSlaveLease&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(finish());
            buffer_ = other.buffer_;
            active_ = other.active_;
            other.buffer_ = {};
            other.active_ = false;
        }
        return *this;
    }

    ~SdioSlaveLease()
    {
        static_cast<void>(finish());
    }

    [[nodiscard]] constexpr bool active() const noexcept
    {
        return active_;
    }

    [[nodiscard]] Result<SdioSlaveTransfer> finish(
        const std::uint32_t timeout_ms = sdio_wait_forever) noexcept
    {
        if (!active_) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        auto transfer = Policy::sdio_finish(timeout_ms);
        if (!transfer) {
            return fail(transfer.error());
        }
        const auto ready = Cache::from_device(buffer_);
        if (ready != ESP_OK) {
            return fail(ready);
        }
        active_ = false;
        return transfer;
    }

private:
    std::span<std::uint8_t> buffer_{};
    bool active_{};
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
        const auto ready = Cache::discard(buffer);
        if (ready != ESP_OK) {
            return Status{fail(ready)};
        }
        return status(Policy::sdio_queue(buffer));
    }

    template <typename Policy>
    [[nodiscard]] static Result<SdioSlaveLease<Policy>> lease_coherent(
        const std::span<std::uint8_t> buffer) noexcept
    {
        auto ready = queue_coherent<Policy>(buffer);
        if (!ready) {
            return fail(ready.error());
        }
        return SdioSlaveLease<Policy>{buffer};
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
