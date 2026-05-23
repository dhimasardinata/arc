#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/swarm.hpp"

namespace arc::net {

struct FabricRoute {
    std::uint32_t dst{};
    std::uint32_t next_hop{};
    std::uint8_t ttl{1U};
};

template <std::size_t MaxBytes>
struct FabricPacket {
    static_assert(MaxBytes > 0U, "fabric packet must have payload storage");

    std::uint32_t src{};
    std::uint32_t dst{};
    std::uint8_t ttl{};
    std::array<std::uint8_t, MaxBytes> payload{};
    std::size_t bytes{};
};

template <std::size_t Routes>
struct FabricTable {
    static_assert(Routes > 0U, "fabric table needs at least one route");

    std::array<FabricRoute, Routes> routes{};
};

struct Fabric {
    template <std::size_t Routes>
    [[nodiscard]] static Result<FabricRoute> lookup(
        const FabricTable<Routes>& table,
        const std::uint32_t dst) noexcept
    {
        for (const auto route : table.routes) {
            if (route.dst != 0U && route.dst == dst && route.next_hop != 0U) {
                return route;
            }
        }
        return fail(ESP_ERR_NOT_FOUND);
    }

    template <std::size_t MaxBytes>
    [[nodiscard]] static Result<FabricPacket<MaxBytes>> make(
        const std::uint32_t src,
        const std::uint32_t dst,
        const std::span<const std::uint8_t> payload,
        const std::uint8_t ttl) noexcept
    {
        if (src == 0U || dst == 0U || ttl == 0U || payload.size() > MaxBytes ||
            (!payload.empty() && payload.data() == nullptr)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        FabricPacket<MaxBytes> packet{.src = src, .dst = dst, .ttl = ttl, .bytes = payload.size()};
        for (std::size_t i = 0U; i < payload.size(); ++i) {
            packet.payload[i] = payload[i];
        }
        return packet;
    }

    template <typename Policy, std::size_t Routes, std::size_t MaxBytes, std::size_t Nodes>
    [[nodiscard]] static Status forward(
        const FabricTable<Routes>& table,
        FabricPacket<MaxBytes>& packet,
        const SwarmSchedule<Nodes>& schedule,
        const std::uint64_t now_us) noexcept
    {
        if (!valid_packet(packet)) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        auto route = lookup(table, packet.dst);
        if (!route) {
            return Status{fail(route.error())};
        }
        if (!schedule.active(route->next_hop, now_us)) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        --packet.ttl;
        return status(Policy::send(route->next_hop, std::span<const std::uint8_t>{packet.payload.data(), packet.bytes}));
    }

    template <std::size_t MaxBytes>
    [[nodiscard]] static constexpr bool valid_packet(
        const FabricPacket<MaxBytes>& packet) noexcept
    {
        return packet.src != 0U && packet.dst != 0U && packet.ttl != 0U && packet.bytes <= MaxBytes;
    }
};

}  // namespace arc::net
