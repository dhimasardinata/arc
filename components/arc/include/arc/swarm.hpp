#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "arc/tdma.hpp"

namespace arc::net {

template <std::size_t Nodes>
struct SwarmSchedule {
    static_assert(Nodes > 0U, "swarm node count must be non-zero");

    std::array<std::uint32_t, Nodes> node_ids{};
    TdmaConfig tdma{};

    [[nodiscard]] constexpr std::uint32_t slot_for(const std::uint32_t node_id) const noexcept
    {
        for (std::uint32_t i = 0; i < Nodes; ++i) {
            if (node_ids[i] == node_id) {
                return i;
            }
        }
        return Nodes;
    }

    [[nodiscard]] constexpr TdmaWindow window(
        const std::uint32_t node_id,
        const std::uint64_t now_us) const noexcept
    {
        auto config = tdma;
        config.slot = slot_for(node_id);
        return Tdma::window(now_us, config);
    }
};

}  // namespace arc::net
