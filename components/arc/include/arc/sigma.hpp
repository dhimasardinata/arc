#pragma once

#include <cstdint>

#include "driver/sdm.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "hal/gpio_types.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin,
          std::uint32_t SampleHz = 1'000'000,
          int Initial = 0,
          bool Invert = false,
          bool AllowPd = false,
          sdm_clock_source_t Source = SDM_CLK_SRC_DEFAULT>
struct Sigma {
    static constexpr int min = -128;
    static constexpr int max = 127;

    static_assert(SOC_SDM_SUPPORTED, "sigma-delta modulator is not supported on this target");
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid GPIO pin number");
    static_assert(SampleHz != 0U, "sigma-delta sample rate must be non-zero");
    static_assert(Initial >= min && Initial <= max, "sigma-delta density must be in [-128, 127]");

    [[nodiscard]] static constexpr gpio_num_t gpio_number() noexcept
    {
        return static_cast<gpio_num_t>(Pin);
    }

    [[nodiscard]] static constexpr std::uint32_t rate() noexcept
    {
        return SampleHz;
    }

    static void boot()
    {
        if (create()) {
            ESP_ERROR_CHECK(fast(static_cast<std::int8_t>(Initial)));
        }
    }

    static void start()
    {
        boot();

        if (!state.enabled) {
            ESP_ERROR_CHECK(sdm_channel_enable(state.channel));
            state.enabled = true;
        }
    }

    static void stop()
    {
        if (state.channel == nullptr || !state.enabled) {
            return;
        }

        ESP_ERROR_CHECK(sdm_channel_disable(state.channel));
        state.enabled = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        start();
    }

    static void write(const int density)
    {
        create();
        ESP_ERROR_CHECK(valid(density) ? ESP_OK : ESP_ERR_INVALID_ARG);
        ESP_ERROR_CHECK(fast(static_cast<std::int8_t>(density)));
    }

    template <int Density>
    static void write()
    {
        static_assert(valid(Density), "sigma-delta density must be in [-128, 127]");
        create();
        ESP_ERROR_CHECK(fast(static_cast<std::int8_t>(Density)));
    }

    static void density(const int value)
    {
        write(value);
    }

    template <int Value>
    static void density()
    {
        write<Value>();
    }

    static void zero()
    {
        write<0>();
    }

    static void high()
    {
        write<max>();
    }

    static void low()
    {
        write<min>();
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline esp_err_t fast(const std::int8_t density) noexcept
    {
        state.density = density;
        return sdm_channel_set_pulse_density(state.channel, density);
    }

    [[nodiscard]] static std::int8_t level() noexcept
    {
        return state.density;
    }

private:
    struct State {
        sdm_channel_handle_t channel{};
        std::int8_t density{static_cast<std::int8_t>(Initial)};
        bool enabled{};
    };

    constinit static inline State state{};

    [[nodiscard]] static constexpr bool valid(const int density) noexcept
    {
        return density >= min && density <= max;
    }

    static bool create()
    {
        if (state.channel != nullptr) {
            return false;
        }

        sdm_config_t config{};
        config.gpio_num = gpio_number();
        config.clk_src = Source;
        config.sample_rate_hz = SampleHz;
        config.flags.invert_out = Invert ? 1U : 0U;
        config.flags.allow_pd = AllowPd ? 1U : 0U;
        ESP_ERROR_CHECK(sdm_new_channel(&config, &state.channel));
        return true;
    }
};

}  // namespace arc
