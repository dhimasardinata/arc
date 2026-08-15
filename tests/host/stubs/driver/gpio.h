#pragma once

#include <cstdint>

#include "esp_err.h"

enum gpio_mode_t : std::uint32_t {
    GPIO_MODE_OUTPUT = 1U,
};

enum gpio_pullup_t : std::uint32_t {
    GPIO_PULLUP_DISABLE = 0U,
};

enum gpio_pulldown_t : std::uint32_t {
    GPIO_PULLDOWN_DISABLE = 0U,
};

enum gpio_int_type_t : std::uint32_t {
    GPIO_INTR_DISABLE = 0U,
};

struct gpio_config_t {
    std::uint64_t pin_bit_mask{};
    gpio_mode_t mode{GPIO_MODE_OUTPUT};
    gpio_pullup_t pull_up_en{GPIO_PULLUP_DISABLE};
    gpio_pulldown_t pull_down_en{GPIO_PULLDOWN_DISABLE};
    gpio_int_type_t intr_type{GPIO_INTR_DISABLE};
};

inline gpio_config_t gpio_last_config{};

inline esp_err_t gpio_config(const gpio_config_t* const config) noexcept
{
    if (config == nullptr || config->pin_bit_mask == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_last_config = *config;
    return ESP_OK;
}
