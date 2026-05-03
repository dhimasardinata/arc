#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

namespace arc {

enum class WatchTrigger : std::uint32_t {
    read = 1U,
    write = 2U,
    access = read | write,
};

struct WatchRegion {
    const void* address{};
    std::size_t bytes{};
    WatchTrigger trigger{WatchTrigger::write};
};

template <typename Policy>
struct HardwareGuard {
    explicit HardwareGuard(const WatchRegion region) noexcept
        : region_(region)
    {
        err_ = Policy::set(region_);
    }

    HardwareGuard(const HardwareGuard&) = delete;
    HardwareGuard& operator=(const HardwareGuard&) = delete;

    ~HardwareGuard()
    {
        if (err_ == ESP_OK) {
            static_cast<void>(Policy::clear(region_));
        }
    }

    [[nodiscard]] esp_err_t status() const noexcept
    {
        return err_;
    }

private:
    WatchRegion region_{};
    esp_err_t err_{ESP_FAIL};
};

}  // namespace arc
