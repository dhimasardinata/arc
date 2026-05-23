#pragma once

#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::ble {

struct MeshAddress {
    std::uint16_t unicast{};
    std::uint16_t group{};
};

struct MeshModel {
    std::uint16_t company{};
    std::uint16_t model{};
};

struct Mesh {
    template <typename Policy>
    [[nodiscard]] static Status provision(const MeshAddress address) noexcept
    {
        if (address.unicast == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::mesh_provision(address));
    }

    template <typename Policy>
    [[nodiscard]] static Status publish(
        const MeshAddress address,
        const MeshModel model,
        const std::span<const std::uint8_t> payload) noexcept
    {
        if (address.group == 0U || model.model == 0U || payload.empty() || payload.data() == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::mesh_publish(address, model, payload));
    }
};

}  // namespace arc::ble
