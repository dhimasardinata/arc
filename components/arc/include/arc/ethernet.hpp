#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "esp_err.h"

#include "arc/result.hpp"
#include "arc/sdk.hpp"
#include "arc/spsc.hpp"

namespace arc::net {

namespace detail {

template <typename CopyEngine>
concept StrictCopyEngine = requires(
    typename CopyEngine::StrictTicket& ticket,
    std::span<std::uint8_t> dst,
    std::span<const std::uint8_t> src) {
    { CopyEngine::send_strict(ticket, dst, src) } -> std::same_as<esp_err_t>;
    { CopyEngine::finish_coherent(ticket) } -> std::same_as<esp_err_t>;
};

}  // namespace detail

template <std::size_t Mtu = 1536U>
struct EthernetFrame {
    static_assert(Mtu >= 64U, "Ethernet MTU must fit a minimum frame");
    static_assert(Mtu <= std::numeric_limits<std::uint16_t>::max(), "Ethernet MTU must fit frame size metadata");

    alignas(cache_line) std::array<std::uint8_t, Mtu> bytes{};
    std::uint16_t size{};

    [[nodiscard]] std::span<const std::uint8_t> view() const noexcept
    {
        return {bytes.data(), size};
    }
};

template <std::size_t Mtu, std::size_t Capacity>
struct EthernetRing {
    using Frame = EthernetFrame<Mtu>;

    [[nodiscard]] bool push(std::span<const std::uint8_t> frame) noexcept
    {
        if (frame.size() > Mtu || (!frame.empty() && frame.data() == nullptr)) {
            return false;
        }
        Frame out{};
        out.size = static_cast<std::uint16_t>(frame.size());
        for (std::size_t i = 0; i < frame.size(); ++i) {
            out.bytes[i] = frame[i];
        }
        return rx.try_push(out);
    }

    template <detail::StrictCopyEngine CopyEngine>
    [[nodiscard]] bool push_strict(
        std::span<const std::uint8_t> frame,
        typename CopyEngine::StrictTicket* const ticket = nullptr) noexcept
    {
        if (frame.size() > Mtu || (!frame.empty() && frame.data() == nullptr)) {
            return false;
        }

        typename CopyEngine::StrictTicket local{};
        const auto pushed = rx.try_push_with([&](Frame& out) noexcept {
            if (!frame.empty()) {
                auto dst = std::span<std::uint8_t>{out.bytes}.first(frame.size());
                if (const auto sent = CopyEngine::send_strict(local, dst, frame); sent != ESP_OK) {
                    return false;
                }
                if (const auto done = CopyEngine::finish_coherent(local); done != ESP_OK) {
                    return false;
                }
            }
            out.size = static_cast<std::uint16_t>(frame.size());
            return true;
        });
        if (!pushed) {
            return false;
        }
        if (ticket != nullptr) {
            *ticket = local;
        }
        return true;
    }

    [[nodiscard]] bool pop(Frame& frame) noexcept
    {
        return rx.try_pop(frame);
    }

    Spsc<Frame, Capacity> rx{};
};

}  // namespace arc::net
