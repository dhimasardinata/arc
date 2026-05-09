#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

namespace arc {

enum class PmsAccess : std::uint32_t {
    none = 0U,
    read = 1U,
    write = 2U,
    execute = 4U,
    read_write = read | write,
    read_execute = read | execute,
    read_write_execute = read | write | execute,
};

struct PmsRegion {
    const void* base{};
    std::size_t bytes{};
    PmsAccess core0{PmsAccess::read_write};
    PmsAccess core1{PmsAccess::read_write};
};

template <typename Policy>
struct Pms {
    [[nodiscard]] static esp_err_t lock(std::span<const PmsRegion> regions) noexcept
    {
        return Policy::lock(regions);
    }

    [[nodiscard]] static esp_err_t unlock() noexcept
    {
        return Policy::unlock();
    }
};

}  // namespace arc
