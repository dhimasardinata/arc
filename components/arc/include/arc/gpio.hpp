#pragma once

#include <cstdint>

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "soc/gpio_struct.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin>
struct Gpio {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid GPIO pin number");

    static constexpr bool low_bank() noexcept
    {
        return Pin < 32;
    }

    static constexpr std::uint32_t mask() noexcept
    {
        return std::uint32_t{1} << (low_bank() ? Pin : (Pin - 32));
    }

    static void out()
    {
        gpio_config_t configuration{};
        configuration.pin_bit_mask = std::uint64_t{1} << Pin;
        configuration.mode = GPIO_MODE_OUTPUT;
        configuration.pull_up_en = GPIO_PULLUP_DISABLE;
        configuration.pull_down_en = GPIO_PULLDOWN_DISABLE;
        configuration.intr_type = GPIO_INTR_DISABLE;

        ESP_ERROR_CHECK(gpio_config(&configuration));
    }

    template <bool Level>
    IRAM_ATTR [[gnu::always_inline]] static inline void write() noexcept
    {
        if constexpr (Level) {
            hi();
        } else {
            lo();
        }
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void enable() noexcept
    {
        if constexpr (low_bank()) {
            GPIO.enable_w1ts = mask();
        } else {
            GPIO.enable1_w1ts.val = mask();
        }
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void disable() noexcept
    {
        if constexpr (low_bank()) {
            GPIO.enable_w1tc = mask();
        } else {
            GPIO.enable1_w1tc.val = mask();
        }
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void hi() noexcept
    {
        if constexpr (low_bank()) {
            GPIO.out_w1ts = mask();
        } else {
            GPIO.out1_w1ts.val = mask();
        }
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void lo() noexcept
    {
        if constexpr (low_bank()) {
            GPIO.out_w1tc = mask();
        } else {
            GPIO.out1_w1tc.val = mask();
        }
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline bool high() noexcept
    {
        if constexpr (low_bank()) {
            return (GPIO.out & mask()) != 0U;
        } else {
            return (GPIO.out1.val & mask()) != 0U;
        }
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void toggle() noexcept
    {
        if (high()) {
            lo();
        } else {
            hi();
        }
    }
};

}  // namespace arc
