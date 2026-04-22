#pragma once

#include <cstdint>

#include "driver/temperature_sensor.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"

namespace arc {

template <int Min = -10,
          int Max = 80,
          bool AllowPd = false,
          temperature_sensor_clk_src_t Source = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT>
struct Temp {
    static_assert(SOC_TEMP_SENSOR_SUPPORTED, "temperature sensor is not supported on this target");
    static_assert(Min < Max, "temperature sensor range must be ordered");

    static void boot()
    {
        create();
    }

    static void start()
    {
        create();

        if (!state.enabled) {
            ESP_ERROR_CHECK(temperature_sensor_enable(state.sensor));
            state.enabled = true;
        }
    }

    static void stop()
    {
        if (state.sensor == nullptr || !state.enabled) {
            return;
        }

        ESP_ERROR_CHECK(temperature_sensor_disable(state.sensor));
        state.enabled = false;
    }

    [[nodiscard]] static esp_err_t read(float& value) noexcept
    {
        create();
        return temperature_sensor_get_celsius(state.sensor, &value);
    }

    [[nodiscard]] static float celsius()
    {
        start();

        float value = 0.0F;
        ESP_ERROR_CHECK(read(value));
        return value;
    }

    [[nodiscard]] static int milli()
    {
        return static_cast<int>(celsius() * 1000.0F);
    }

    [[nodiscard]] static temperature_sensor_handle_t native()
    {
        create();
        return state.sensor;
    }

private:
    struct State {
        temperature_sensor_handle_t sensor{};
        bool enabled{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    static void create()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(Min, Max);
        config.clk_src = Source;
        config.flags.allow_pd = AllowPd ? 1U : 0U;
        ESP_ERROR_CHECK(temperature_sensor_install(&config, &state.sensor));
        Init::pass(state.init);
    }
};

}  // namespace arc
