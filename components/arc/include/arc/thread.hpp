#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::net {

struct ThreadDataset {
    std::array<std::uint8_t, 16> network_key{};
    std::uint16_t pan_id{};
    std::uint8_t channel{15U};
};

struct ThreadPeer {
    std::array<std::uint8_t, 16> mesh_local{};
    std::uint16_t rloc16{};
};

struct Thread {
    template <typename Policy>
    [[nodiscard]] static Status attach(const ThreadDataset& dataset) noexcept
    {
        if (dataset.pan_id == 0U || dataset.channel < 11U || dataset.channel > 26U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::thread_attach(dataset));
    }

    template <typename Policy>
    [[nodiscard]] static Status send(
        const ThreadPeer& peer,
        const std::span<const std::uint8_t> payload) noexcept
    {
        if (peer.rloc16 == 0U || payload.empty() || payload.data() == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::thread_send(peer, payload));
    }
};

}  // namespace arc::net
