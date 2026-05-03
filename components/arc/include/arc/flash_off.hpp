#pragma once

#include "esp_err.h"

namespace arc {

template <typename Policy>
struct FlashOff {
    [[nodiscard]] esp_err_t enter() noexcept
    {
        if (active) {
            return ESP_ERR_INVALID_STATE;
        }
        const auto err = Policy::enter();
        if (err == ESP_OK) {
            active = true;
        }
        return err;
    }

    [[nodiscard]] esp_err_t leave() noexcept
    {
        if (!active) {
            return ESP_OK;
        }
        const auto err = Policy::leave();
        if (err == ESP_OK) {
            active = false;
        }
        return err;
    }

    [[nodiscard]] bool on() const noexcept
    {
        return active;
    }

    bool active{};
};

}  // namespace arc
