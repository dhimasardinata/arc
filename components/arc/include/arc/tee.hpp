#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

#include "arc/pms.hpp"

namespace arc {

enum class World : std::uint32_t {
    trusted = 0U,
    untrusted = 1U,
};

struct WorldRegion {
    const void* base{};
    std::size_t bytes{};
    World owner{World::trusted};
    PmsAccess trusted{PmsAccess::read_write};
    PmsAccess untrusted{PmsAccess::none};
};

template <typename Policy>
struct WorldGuard {
    [[nodiscard]] static esp_err_t configure(std::span<const WorldRegion> regions) noexcept
    {
        return Policy::configure(regions);
    }

    [[nodiscard]] static esp_err_t core_world(
        const std::uint32_t core,
        const World world) noexcept
    {
        return Policy::core_world(core, world);
    }

    [[nodiscard]] static esp_err_t peripheral_world(
        const std::uint32_t peripheral,
        const World world) noexcept
    {
        return Policy::peripheral_world(peripheral, world);
    }
};

}  // namespace arc
