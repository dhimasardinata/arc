#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::net {

struct RdmaWindow {
    std::uint32_t node{};
    std::uintptr_t base{};
    std::size_t bytes{};
    std::uint16_t alignment{16U};
};

struct RdmaWrite {
    std::uint32_t src{};
    std::uint32_t dst{};
    std::uint32_t offset{};
    std::uint16_t bytes{};
    std::uint16_t flags{};
};

template <std::size_t MaxBytes>
struct RdmaFrame {
    static_assert(MaxBytes > 0U, "RDMA frame needs payload storage");

    RdmaWrite header{};
    std::array<std::uint8_t, MaxBytes> payload{};
};

struct Rdma {
    [[nodiscard]] static constexpr bool aligned(const RdmaWindow window) noexcept
    {
        return window.alignment != 0U &&
            (window.alignment & (window.alignment - 1U)) == 0U &&
            (window.base & (window.alignment - 1U)) == 0U;
    }

    [[nodiscard]] static constexpr bool range_fit(const RdmaWindow window) noexcept
    {
        constexpr auto max_address = std::numeric_limits<std::uintptr_t>::max();
        return window.bytes <= max_address &&
            window.base <= (max_address - static_cast<std::uintptr_t>(window.bytes));
    }

    template <std::size_t MaxBytes>
    [[nodiscard]] static Result<RdmaFrame<MaxBytes>> write(
        const RdmaWindow remote,
        const std::uint32_t src,
        const std::span<const std::uint8_t> bytes,
        const std::uint32_t offset = 0U) noexcept
    {
        if (src == 0U || remote.node == 0U || bytes.empty() || bytes.size() > MaxBytes ||
            bytes.size() > std::numeric_limits<std::uint16_t>::max() ||
            bytes.data() == nullptr ||
            !aligned(remote) || !range_fit(remote) || offset > remote.bytes || bytes.size() > remote.bytes - offset) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        RdmaFrame<MaxBytes> frame{
            .header = {
                .src = src,
                .dst = remote.node,
                .offset = offset,
                .bytes = static_cast<std::uint16_t>(bytes.size()),
            },
        };
        for (std::size_t i = 0U; i < bytes.size(); ++i) {
            frame.payload[i] = bytes[i];
        }
        return frame;
    }

    template <typename Policy, std::size_t MaxBytes>
    [[nodiscard]] static Status apply(
        const RdmaWindow local,
        const RdmaFrame<MaxBytes>& frame) noexcept
    {
        if (local.node == 0U || frame.header.src == 0U || frame.header.bytes == 0U ||
            frame.header.dst != local.node || frame.header.bytes > MaxBytes ||
            frame.header.offset > local.bytes || frame.header.bytes > local.bytes - frame.header.offset ||
            !aligned(local) || !range_fit(local)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        const auto dst = local.base + frame.header.offset;
        const auto payload = std::span<const std::uint8_t>{frame.payload.data(), frame.header.bytes};
        return status(Policy::store(dst, payload));
    }

    template <typename Policy>
    [[nodiscard]] static Status promiscuous(bool enable = true) noexcept
    {
        return status(Policy::promiscuous(enable));
    }
};

}  // namespace arc::net
