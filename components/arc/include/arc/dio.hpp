#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_private/gpio.h"
#include "esp_rom_gpio.h"
#include "hal/dedic_gpio_caps.h"
#include "hal/dedic_gpio_cpu_ll.h"
#include "hal/dedic_gpio_periph.h"

#include "arc/task.hpp"

namespace arc {

template <int Pin, std::uint32_t Chan, Core Bind = Core::core1>
struct Dio {
    static_assert(Bind != Core::any, "dedicated GPIO must be bound to a specific core");
    static_assert(Chan < DEDIC_GPIO_CAPS_GET(OUT_CHANS_PER_CPU), "invalid dedicated GPIO channel");

    [[nodiscard]] static constexpr gpio_num_t gpio_number() noexcept
    {
        return static_cast<gpio_num_t>(Pin);
    }

    [[nodiscard]] static constexpr std::uint32_t channel_mask() noexcept
    {
        return std::uint32_t{1} << Chan;
    }

    [[nodiscard]] static std::uint32_t sig() noexcept
    {
        return static_cast<std::uint32_t>(
            dedic_gpio_periph_signals.cores[static_cast<int>(Bind)].out_sig_per_channel[Chan]);
    }

    static void out()
    {
        ESP_ERROR_CHECK(gpio_func_sel(gpio_number(), PIN_FUNC_GPIO));
        esp_rom_gpio_connect_out_signal(static_cast<std::uint32_t>(Pin), sig(), false, false);
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

    IRAM_ATTR [[gnu::always_inline]] static inline void hi() noexcept
    {
        dedic_gpio_cpu_ll_write_mask(channel_mask(), channel_mask());
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void on() noexcept
    {
        hi();
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void lo() noexcept
    {
        dedic_gpio_cpu_ll_write_mask(channel_mask(), 0U);
    }

    IRAM_ATTR [[gnu::always_inline]] static inline void off() noexcept
    {
        lo();
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline bool high() noexcept
    {
        return (dedic_gpio_cpu_ll_read_out() & channel_mask()) != 0U;
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

template <int Pin, std::uint32_t Chan, Core Bind = Core::core1>
using Drive = Dio<Pin, Chan, Bind>;

}  // namespace arc
