#pragma once

#include <cstddef>

#include "esp_err.h"

namespace arc {

struct CacheRegion {
    const void* address{};
    std::size_t bytes{};
};

template <typename Policy>
struct CacheLock {
    [[nodiscard]] esp_err_t lock_code(const CacheRegion region) noexcept
    {
        return Policy::lock_code(region);
    }

    [[nodiscard]] esp_err_t lock_data(const CacheRegion region) noexcept
    {
        return Policy::lock_data(region);
    }

    [[nodiscard]] esp_err_t unlock_all() noexcept
    {
        return Policy::unlock_all();
    }
};

}  // namespace arc
