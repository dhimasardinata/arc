#pragma once

#include <cstdint>

#include "driver/pulse_cnt.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int EdgePin,
          int LevelPin = -1,
          int ClearPin = -1,
          int Low = -32768,
          int High = 32767,
          pcnt_channel_edge_action_t Rise = PCNT_CHANNEL_EDGE_ACTION_INCREASE,
          pcnt_channel_edge_action_t Fall = PCNT_CHANNEL_EDGE_ACTION_HOLD,
          pcnt_channel_level_action_t HighMode = PCNT_CHANNEL_LEVEL_ACTION_KEEP,
          pcnt_channel_level_action_t LowMode = PCNT_CHANNEL_LEVEL_ACTION_KEEP,
          std::uint32_t FilterNs = 0,
          bool Accum = true,
          pcnt_clock_source_t Source = PCNT_CLK_SRC_DEFAULT>
struct Count {
    static_assert(EdgePin >= 0 && EdgePin < SOC_GPIO_PIN_COUNT, "invalid PCNT edge pin");
    static_assert(LevelPin < SOC_GPIO_PIN_COUNT, "invalid PCNT level pin");
    static_assert(ClearPin < SOC_GPIO_PIN_COUNT, "invalid PCNT clear pin");
    static_assert(Low < 0, "PCNT low limit must be negative");
    static_assert(High > 0, "PCNT high limit must be positive");
    static_assert(Low < High, "PCNT low limit must be lower than high limit");

    static void start()
    {
        init();

        if (!state.enabled) {
            ESP_ERROR_CHECK(pcnt_unit_enable(state.unit));
            state.enabled = true;
        }

        if (!state.running) {
            ESP_ERROR_CHECK(pcnt_unit_start(state.unit));
            state.running = true;
        }
    }

    static void stop()
    {
        if (!state.running) {
            return;
        }

        ESP_ERROR_CHECK(pcnt_unit_stop(state.unit));
        state.running = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        start();
    }

    static void clear()
    {
        init();
        ESP_ERROR_CHECK(pcnt_unit_clear_count(state.unit));
    }

    [[nodiscard]] static int read()
    {
        init();

        int value = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(state.unit, &value));
        return value;
    }

    static void watch(const int point)
    {
        init();
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(state.unit, point));
    }

    static void unwatch(const int point)
    {
        init();
        ESP_ERROR_CHECK(pcnt_unit_remove_watch_point(state.unit, point));
    }

private:
    struct State {
        pcnt_unit_handle_t unit{};
        pcnt_channel_handle_t chan{};
        bool enabled{};
        bool running{};
    };

    constinit static inline State state{};

    static void init()
    {
        if (state.unit != nullptr) {
            return;
        }

        pcnt_unit_config_t unit_cfg{};
        unit_cfg.clk_src = Source;
        unit_cfg.low_limit = Low;
        unit_cfg.high_limit = High;
        unit_cfg.intr_priority = 0;
        unit_cfg.flags.accum_count = Accum ? 1U : 0U;
        ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &state.unit));

        pcnt_chan_config_t chan_cfg{};
        chan_cfg.edge_gpio_num = EdgePin;
        chan_cfg.level_gpio_num = LevelPin >= 0 ? LevelPin : -1;
        chan_cfg.flags.invert_edge_input = 0;
        chan_cfg.flags.invert_level_input = 0;
        chan_cfg.flags.virt_edge_io_level = 0;
        chan_cfg.flags.virt_level_io_level = LevelPin >= 0 ? 0U : 1U;
        ESP_ERROR_CHECK(pcnt_new_channel(state.unit, &chan_cfg, &state.chan));

        ESP_ERROR_CHECK(pcnt_channel_set_edge_action(state.chan, Rise, Fall));
        ESP_ERROR_CHECK(pcnt_channel_set_level_action(state.chan, HighMode, LowMode));

        if constexpr (FilterNs != 0U) {
            pcnt_glitch_filter_config_t filter{};
            filter.max_glitch_ns = FilterNs;
            ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(state.unit, &filter));
        }

#if SOC_PCNT_SUPPORT_CLEAR_SIGNAL
        if constexpr (ClearPin >= 0) {
            pcnt_clear_signal_config_t clear{};
            clear.clear_signal_gpio_num = ClearPin;
            clear.flags.invert_clear_signal = 0;
            ESP_ERROR_CHECK(pcnt_unit_set_clear_signal(state.unit, &clear));
        }
#endif

        ESP_ERROR_CHECK(pcnt_unit_clear_count(state.unit));
    }
};

}  // namespace arc
