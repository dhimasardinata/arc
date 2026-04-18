#pragma once

#include <cstdint>

#include "driver/ledc.h"
#include "esp_check.h"
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

    [[nodiscard]] static constexpr std::uint32_t max() noexcept
    {
        return static_cast<std::uint32_t>((std::uint64_t{1} << Bits) - 1ULL);
    }

    static void start()
    {
        ledc_timer_config_t timer_cfg{};
        timer_cfg.speed_mode = Mode;
        timer_cfg.duty_resolution = resolution();
        timer_cfg.timer_num = timer();
        timer_cfg.freq_hz = Hz;
        timer_cfg.clk_cfg = LEDC_AUTO_CLK;
        timer_cfg.deconfigure = false;
        ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

        ledc_channel_config_t channel_cfg{};
        channel_cfg.gpio_num = Pin;
        channel_cfg.speed_mode = Mode;
        channel_cfg.channel = channel();
        channel_cfg.timer_sel = timer();
        channel_cfg.duty = raw(DutyPermille);
        channel_cfg.hpoint = 0;
        channel_cfg.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
        channel_cfg.flags.output_invert = 0;
        channel_cfg.deconfigure = false;
        ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
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
        apply(raw(DutyPermille));
    }

    static void off()
    {
        ESP_ERROR_CHECK(ledc_stop(Mode, channel(), 0U));
    }

    template <std::uint32_t Permille>
    static void duty()
    {
        static_assert(Permille <= 1000U, "PWM duty must be in permille");
        apply(raw(Permille));
    }

private:
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

    static void apply(const std::uint32_t duty) noexcept
    {
        ESP_ERROR_CHECK(ledc_set_duty_and_update(Mode, channel(), duty, 0U));
    }
};

}  // namespace arc
