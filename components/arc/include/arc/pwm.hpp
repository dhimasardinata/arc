#pragma once

#include <cstdint>

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin,
          std::uint32_t Hz,
          std::uint32_t DutyPermille = 500,
          std::uint32_t Chan = 0,
          std::uint32_t Timer = (Chan % SOC_LEDC_TIMER_NUM),
          std::uint32_t Bits = 10,
          ledc_mode_t Mode = LEDC_LOW_SPEED_MODE>
struct Pwm {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid GPIO pin number");
    static_assert(Hz != 0U, "PWM frequency must be non-zero");
    static_assert(DutyPermille <= 1000U, "PWM duty must be in permille");
    static_assert(Chan < SOC_LEDC_CHANNEL_NUM, "invalid LEDC channel");
    static_assert(Timer < SOC_LEDC_TIMER_NUM, "invalid LEDC timer");
    static_assert(Bits >= 1U && Bits <= SOC_LEDC_TIMER_BIT_WIDTH, "invalid LEDC duty resolution");

    struct Config {
        std::uint32_t hz{Hz};
        std::uint32_t duty{DutyPermille};
    };

    [[nodiscard]] static constexpr gpio_num_t gpio_number() noexcept
    {
        return static_cast<gpio_num_t>(Pin);
    }

    [[nodiscard]] static constexpr std::uint32_t frequency() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static constexpr std::uint32_t duty_permille() noexcept
    {
        return DutyPermille;
    }

    [[nodiscard]] static constexpr Config defaults() noexcept
    {
        return Config{};
    }

    [[nodiscard]] static std::uint32_t hz() noexcept
    {
        return state.hz;
    }

    [[nodiscard]] static std::uint32_t permille() noexcept
    {
        return state.duty;
    }

    [[nodiscard]] static Config config() noexcept
    {
        return state;
    }

    [[nodiscard]] static constexpr std::uint32_t max() noexcept
    {
        return static_cast<std::uint32_t>((std::uint64_t{1} << Bits) - 1ULL);
    }

    [[nodiscard]] static esp_err_t begin() noexcept
    {
        return begin(defaults());
    }

    [[nodiscard]] static esp_err_t begin(const Config& cfg) noexcept
    {
        if (cfg.hz == 0U || cfg.duty > 1000U) {
            return ESP_ERR_INVALID_ARG;
        }

        ledc_timer_config_t timer_cfg{};
        timer_cfg.speed_mode = Mode;
        timer_cfg.duty_resolution = resolution();
        timer_cfg.timer_num = timer();
        timer_cfg.freq_hz = cfg.hz;
        timer_cfg.clk_cfg = LEDC_AUTO_CLK;
        timer_cfg.deconfigure = false;
        auto err = ledc_timer_config(&timer_cfg);
        if (err != ESP_OK) {
            return err;
        }

        ledc_channel_config_t channel_cfg{};
        channel_cfg.gpio_num = Pin;
        channel_cfg.speed_mode = Mode;
        channel_cfg.channel = channel();
        channel_cfg.timer_sel = timer();
        channel_cfg.duty = raw(cfg.duty);
        channel_cfg.hpoint = 0;
        channel_cfg.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
        channel_cfg.flags.output_invert = 0;
        channel_cfg.deconfigure = false;
        err = ledc_channel_config(&channel_cfg);
        if (err != ESP_OK) {
            return err;
        }

        state = cfg;
        return ESP_OK;
    }

    static void start()
    {
        ESP_ERROR_CHECK(begin());
    }

    static void start(const Config& cfg)
    {
        ESP_ERROR_CHECK(begin(cfg));
    }

    static void pause()
    {
        ESP_ERROR_CHECK(ledc_timer_pause(Mode, timer()));
    }

    static void resume()
    {
        ESP_ERROR_CHECK(ledc_timer_resume(Mode, timer()));
    }

    static void on()
    {
        ESP_ERROR_CHECK(duty(permille()));
    }

    static void off()
    {
        ESP_ERROR_CHECK(ledc_stop(Mode, channel(), 0U));
    }

    template <std::uint32_t Permille>
    static void duty()
    {
        static_assert(Permille <= 1000U, "PWM duty must be in permille");
        ESP_ERROR_CHECK(duty(Permille));
    }

    [[nodiscard]] static esp_err_t duty(const std::uint32_t permille) noexcept
    {
        if (permille > 1000U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto err = apply(raw(permille));
        if (err == ESP_OK) {
            state.duty = permille;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t set(const std::uint32_t permille) noexcept
    {
        return duty(permille);
    }

    [[nodiscard]] static esp_err_t hz(const std::uint32_t value) noexcept
    {
        return begin(Config{
            .hz = value,
            .duty = permille(),
        });
    }

private:
    constinit static inline Config state{};

    [[nodiscard]] static constexpr ledc_channel_t channel() noexcept
    {
        return static_cast<ledc_channel_t>(Chan);
    }

    [[nodiscard]] static constexpr ledc_timer_t timer() noexcept
    {
        return static_cast<ledc_timer_t>(Timer);
    }

    [[nodiscard]] static constexpr ledc_timer_bit_t resolution() noexcept
    {
        return static_cast<ledc_timer_bit_t>(Bits);
    }

    [[nodiscard]] static constexpr std::uint32_t raw(const std::uint32_t permille) noexcept
    {
        return static_cast<std::uint32_t>((static_cast<std::uint64_t>(max()) * permille + 500ULL) / 1000ULL);
    }

    [[nodiscard]] static esp_err_t apply(const std::uint32_t duty) noexcept
    {
        return ledc_set_duty_and_update(Mode, channel(), duty, 0U);
    }
};

}  // namespace arc
