#pragma once

#include <cstdint>

#include "driver/temperature_sensor.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"
#include "arc/result.hpp"

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
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(Min, Max);
        config.clk_src = Source;
        config.flags.allow_pd = AllowPd ? 1U : 0U;
        const auto err = temperature_sensor_install(&config, &state.sensor);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            state.sensor = nullptr;
            Init::fail(state.init);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        if (!state.enabled) {
            const auto err = temperature_sensor_enable(state.sensor);
            if (err != ESP_OK) {
                return err;
            }
            state.enabled = true;
        }
        return ESP_OK;
    }

    static void start()
    {
        ESP_ERROR_CHECK(enable());
    }

    [[nodiscard]] static esp_err_t disable() noexcept
    {
        if (state.sensor == nullptr || !state.enabled) {
            return ESP_OK;
        }

        const auto err = temperature_sensor_disable(state.sensor);
        if (err != ESP_OK) {
            return err;
        }
        state.enabled = false;
        return ESP_OK;
    }

    static void stop()
    {
        ESP_ERROR_CHECK(disable());
    }

    [[nodiscard]] static esp_err_t read(float& value) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return temperature_sensor_get_celsius(state.sensor, &value);
    }

    [[nodiscard]] static Result<float> read() noexcept
    {
        float value = 0.0F;
        const auto err = read(value);
        if (err != ESP_OK) {
            return fail(err);
        }
        return value;
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
        boot();
        return state.sensor;
    }

private:
    struct State {
        temperature_sensor_handle_t sensor{};
        bool enabled{};
        std::uint32_t init{};
    };

    constinit static inline State state{};
};

}  // namespace arc
