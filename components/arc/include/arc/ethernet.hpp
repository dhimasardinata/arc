#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "esp_err.h"

#include "arc/result.hpp"
#include "arc/spsc.hpp"

namespace arc::net {

template <std::size_t Mtu = 1536U>
struct EthernetFrame {
    static_assert(Mtu >= 64U, "Ethernet MTU must fit a minimum frame");
    static_assert(Mtu <= std::numeric_limits<std::uint16_t>::max(), "Ethernet MTU must fit frame size metadata");

    std::array<std::uint8_t, Mtu> bytes{};
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

    [[nodiscard]] bool pop(Frame& frame) noexcept
    {
        return rx.try_pop(frame);
    }

    Spsc<Frame, Capacity> rx{};
};

}  // namespace arc::net
