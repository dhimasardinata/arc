#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::usb {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool host_valid_span(const std::span<T, Extent> data) noexcept
{
    return data.empty() || data.data() != nullptr;
}

}  // namespace detail

struct HostConfig {
    std::uint8_t port{1U};
    std::uint16_t max_packet{64U};
    bool low_speed{};
};

struct HostTransfer {
    std::uint8_t address{};
    std::uint8_t endpoint{};
    std::span<const std::uint8_t> tx{};
    std::span<std::uint8_t> rx{};
    std::uint32_t timeout_ms{100U};
};

struct HostDevice {
    std::uint8_t address{};
    std::uint16_t vid{};
    std::uint16_t pid{};
    bool configured{};
};

struct Host {
    template <typename Policy>
    [[nodiscard]] static Status start(const HostConfig config = {}) noexcept
    {
        if (config.port == 0U || config.max_packet == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::host_start(config));
    }

    template <typename Policy>
    [[nodiscard]] static Result<HostDevice> poll(const std::uint32_t timeout_ms = 0U) noexcept
    {
        return Policy::host_poll(timeout_ms);
    }

    template <typename Policy>
    [[nodiscard]] static Result<std::size_t> submit(const HostTransfer transfer) noexcept
    {
        if (transfer.address == 0U || transfer.endpoint == 0U ||
            (transfer.tx.empty() && transfer.rx.empty()) ||
            !detail::host_valid_span(transfer.tx) || !detail::host_valid_span(transfer.rx)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return Policy::host_submit(transfer);
    }

    template <typename Policy>
    [[nodiscard]] static Status stop() noexcept
    {
        return status(Policy::host_stop());
    }
};

}  // namespace arc::usb
