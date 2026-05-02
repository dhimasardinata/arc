#pragma once

#include <cstdint>

#include "driver/touch_sens.h"
#include "esp_check.h"
#include "soc/soc_caps.h"
#include "soc/touch_sensor_channel.h"

#include "arc/claim.hpp"
#include "arc/init.hpp"
#include "arc/result.hpp"

namespace arc {

template <std::uint32_t PowerOnWaitUs = 256,
          std::uint32_t MeasIntervalUs = 32,
          std::uint32_t MaxMeasTimeUs = 0,
          std::uint32_t ChargeTimes = 500,
          touch_volt_lim_l_t VoltLow = TOUCH_VOLT_LIM_L_0V5,
          touch_volt_lim_h_t VoltHigh = TOUCH_VOLT_LIM_H_2V2,
          touch_idle_conn_t Idle = TOUCH_IDLE_CONN_GND,
          touch_bias_type_t Bias = TOUCH_BIAS_TYPE_SELF>
struct TouchBus {
    static_assert(SOC_TOUCH_SENSOR_SUPPORTED, "touch sensor is not supported on this target");
    static_assert(SOC_TOUCH_SENSOR_VERSION == 2, "Arc TouchBus currently targets ESP32-S3 touch version 2");
    static_assert(TOUCH_SAMPLE_CFG_NUM == 1, "Arc TouchBus expects a single touch sample configuration on ESP32-S3");
    static_assert(MeasIntervalUs != 0U, "touch measurement interval must be non-zero");
    static_assert(ChargeTimes != 0U, "touch charge times must be non-zero");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        touch_sensor_sample_config_t sample{};
        sample.charge_times = ChargeTimes;
        sample.charge_volt_lim_h = VoltHigh;
        sample.charge_volt_lim_l = VoltLow;
        sample.idle_conn = Idle;
        sample.bias_type = Bias;

        touch_sensor_config_t cfg{};
        cfg.power_on_wait_us = PowerOnWaitUs;
        cfg.meas_interval_us = static_cast<float>(MeasIntervalUs);
        cfg.max_meas_time_us = MaxMeasTimeUs;
        cfg.sample_cfg_num = 1U;
        cfg.sample_cfg = &sample;

        err = touch_sensor_new_controller(&cfg, &state.handle);
        if (err != ESP_OK) {
            state.handle = nullptr;
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        state.enabled = false;
        state.scanning = false;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static touch_sensor_handle_t native() noexcept
    {
        boot();
        return state.handle;
    }

    [[nodiscard]] static esp_err_t config(const touch_sensor_config_t& cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_reconfig_controller(state.handle, &cfg);
    }

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        const auto out = touch_sensor_enable(state.handle);
        if (out == ESP_OK) {
            state.enabled = true;
            state.scanning = false;
        }
        return out;
    }

    [[nodiscard]] static esp_err_t disable() noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        const auto out = touch_sensor_disable(state.handle);
        if (out == ESP_OK) {
            state.enabled = false;
            state.scanning = false;
        }
        return out;
    }

    [[nodiscard]] static esp_err_t start() noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        const auto out = touch_sensor_start_continuous_scanning(state.handle);
        if (out == ESP_OK) {
            state.enabled = true;
            state.scanning = true;
        }
        return out;
    }

    [[nodiscard]] static esp_err_t stop() noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        const auto out = touch_sensor_stop_continuous_scanning(state.handle);
        if (out == ESP_OK) {
            state.enabled = true;
            state.scanning = false;
        }
        return out;
    }

    [[nodiscard]] static esp_err_t scan(const int timeout_ms = -1) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        const auto out = touch_sensor_trigger_oneshot_scanning(state.handle, timeout_ms);
        if (out == ESP_OK) {
            state.enabled = true;
            state.scanning = false;
        }
        return out;
    }

    [[nodiscard]] static esp_err_t filter(const touch_sensor_filter_config_t* const cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_config_filter(state.handle, cfg);
    }

    [[nodiscard]] static esp_err_t events(
        const touch_event_callbacks_t* const callbacks,
        void* const user_ctx = nullptr) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_register_callbacks(state.handle, callbacks, user_ctx);
    }

#if SOC_TOUCH_SUPPORT_SLEEP_WAKEUP
    [[nodiscard]] static esp_err_t sleep(const touch_sleep_config_t* const cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_config_sleep_wakeup(state.handle, cfg);
    }
#endif

#if SOC_TOUCH_SUPPORT_WATERPROOF
    [[nodiscard]] static esp_err_t waterproof(const touch_waterproof_config_t* const cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_config_waterproof(state.handle, cfg);
    }
#endif

#if SOC_TOUCH_SUPPORT_PROX_SENSING
    [[nodiscard]] static esp_err_t proximity(const touch_proximity_config_t* const cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_config_proximity_sensing(state.handle, cfg);
    }
#endif

#if SOC_TOUCH_SUPPORT_DENOISE_CHAN
    [[nodiscard]] static esp_err_t denoise(const touch_denoise_chan_config_t* const cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_config_denoise_channel(state.handle, cfg);
    }
#endif

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        if (state.scanning) {
            const auto err = touch_sensor_stop_continuous_scanning(state.handle);
            if (err != ESP_OK) {
                Init::pass(state.init);
                return err;
            }
            state.scanning = false;
        }

        if (state.enabled) {
            const auto err = touch_sensor_disable(state.handle);
            if (err != ESP_OK) {
                Init::pass(state.init);
                return err;
            }
            state.enabled = false;
        }

        const auto err = touch_sensor_del_controller(state.handle);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.handle = nullptr;
        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

private:
    using Resource = ClaimFor<ClaimKind::touch_bus,
                              0,
                              PowerOnWaitUs,
                              MeasIntervalUs,
                              MaxMeasTimeUs,
                              ChargeTimes,
                              VoltLow,
                              VoltHigh,
                              Idle,
                              Bias>;

    struct State {
        touch_sensor_handle_t handle;
        std::uint32_t init;
        bool enabled;
        bool scanning;
    };

    constinit static inline State state{nullptr, 0U, false, false};
};

template <typename Bus,
          int Io,
          std::uint32_t Active = 2000,
          touch_charge_speed_t Speed = TOUCH_CHARGE_SPEED_7,
          touch_init_charge_volt_t InitVolt = TOUCH_INIT_CHARGE_VOLT_DEFAULT>
struct Touch {
    static_assert(SOC_TOUCH_SENSOR_SUPPORTED, "touch sensor is not supported on this target");
    static_assert(SOC_TOUCH_SENSOR_VERSION == 2, "Arc Touch currently targets ESP32-S3 touch version 2");
    static_assert(TOUCH_SAMPLE_CFG_NUM == 1, "Arc Touch expects a single touch sample configuration on ESP32-S3");
    static_assert(Io >= TOUCH_PAD_NUM1_GPIO_NUM && Io <= TOUCH_PAD_NUM14_GPIO_NUM, "touch GPIO must be in the ESP32-S3 touch pad range");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Bus::init();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        touch_channel_config_t cfg{};
        cfg.active_thresh[0] = Active;
        cfg.charge_speed = Speed;
        cfg.init_charge_volt = InitVolt;

        err = touch_sensor_new_channel(Bus::native(), channel(), &cfg, &state.handle);
        if (err != ESP_OK) {
            state.handle = nullptr;
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static touch_channel_handle_t native() noexcept
    {
        boot();
        return state.handle;
    }

    [[nodiscard]] static esp_err_t config(const touch_channel_config_t& cfg) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_reconfig_channel(state.handle, &cfg);
    }

    [[nodiscard]] static esp_err_t threshold(
        const std::uint32_t active,
        const touch_charge_speed_t speed = Speed,
        const touch_init_charge_volt_t init_volt = InitVolt) noexcept
    {
        touch_channel_config_t cfg{};
        cfg.active_thresh[0] = active;
        cfg.charge_speed = speed;
        cfg.init_charge_volt = init_volt;
        return config(cfg);
    }

    [[nodiscard]] static esp_err_t raw(std::uint32_t& out) noexcept
    {
        return read(TOUCH_CHAN_DATA_TYPE_RAW, out);
    }

    [[nodiscard]] static Result<std::uint32_t> raw() noexcept
    {
        std::uint32_t out{};
        const auto err = raw(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t smooth(std::uint32_t& out) noexcept
    {
        return read(TOUCH_CHAN_DATA_TYPE_SMOOTH, out);
    }

    [[nodiscard]] static Result<std::uint32_t> smooth() noexcept
    {
        std::uint32_t out{};
        const auto err = smooth(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

#if SOC_TOUCH_SUPPORT_BENCHMARK
    [[nodiscard]] static esp_err_t benchmark(std::uint32_t& out) noexcept
    {
        return read(TOUCH_CHAN_DATA_TYPE_BENCHMARK, out);
    }

    [[nodiscard]] static Result<std::uint32_t> benchmark() noexcept
    {
        std::uint32_t out{};
        const auto err = benchmark(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }
#endif

#if SOC_TOUCH_SUPPORT_PROX_SENSING
    [[nodiscard]] static esp_err_t proximity(std::uint32_t& out) noexcept
    {
        return read(TOUCH_CHAN_DATA_TYPE_PROXIMITY, out);
    }

    [[nodiscard]] static Result<std::uint32_t> proximity() noexcept
    {
        std::uint32_t out{};
        const auto err = proximity(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }
#endif

    [[nodiscard]] static esp_err_t info(touch_chan_info_t& out) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return touch_sensor_get_channel_info(state.handle, &out);
    }

    [[nodiscard]] static Result<touch_chan_info_t> info() noexcept
    {
        touch_chan_info_t out{};
        const auto err = info(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = touch_sensor_del_channel(state.handle);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.handle = nullptr;
        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static constexpr int io() noexcept
    {
        return Io;
    }

    [[nodiscard]] static constexpr int channel() noexcept
    {
        if constexpr (Io == TOUCH_PAD_NUM1_GPIO_NUM) {
            return TOUCH_PAD_GPIO1_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM2_GPIO_NUM) {
            return TOUCH_PAD_GPIO2_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM3_GPIO_NUM) {
            return TOUCH_PAD_GPIO3_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM4_GPIO_NUM) {
            return TOUCH_PAD_GPIO4_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM5_GPIO_NUM) {
            return TOUCH_PAD_GPIO5_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM6_GPIO_NUM) {
            return TOUCH_PAD_GPIO6_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM7_GPIO_NUM) {
            return TOUCH_PAD_GPIO7_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM8_GPIO_NUM) {
            return TOUCH_PAD_GPIO8_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM9_GPIO_NUM) {
            return TOUCH_PAD_GPIO9_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM10_GPIO_NUM) {
            return TOUCH_PAD_GPIO10_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM11_GPIO_NUM) {
            return TOUCH_PAD_GPIO11_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM12_GPIO_NUM) {
            return TOUCH_PAD_GPIO12_CHANNEL;
        } else if constexpr (Io == TOUCH_PAD_NUM13_GPIO_NUM) {
            return TOUCH_PAD_GPIO13_CHANNEL;
        } else {
            return TOUCH_PAD_GPIO14_CHANNEL;
        }
    }

private:
    [[nodiscard]] static esp_err_t read(const touch_chan_data_type_t type, std::uint32_t& out) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }

        std::uint32_t data[TOUCH_SAMPLE_CFG_NUM]{};
        const auto read_err = touch_channel_read_data(state.handle, type, data);
        if (read_err == ESP_OK) {
            out = data[0];
        }
        return read_err;
    }

    using Resource = ClaimFor<ClaimKind::touch_chan, Io, Io, Active, Speed, InitVolt>;

    struct State {
        touch_channel_handle_t handle;
        std::uint32_t init;
    };

    constinit static inline State state{nullptr, 0U};
};

}  // namespace arc
