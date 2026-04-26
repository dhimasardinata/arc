#pragma once

#include <cstdint>

#include "driver/pulse_cnt.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/detail/cold.hpp"
#include "arc/init.hpp"

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
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[gnu::cold]] static void init()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        ESP_ERROR_CHECK(detail::cold::count_create(
            {
                .edge_pin = EdgePin,
                .level_pin = LevelPin,
                .clear_pin = ClearPin,
                .low = Low,
                .high = High,
                .rise = Rise,
                .fall = Fall,
                .high_mode = HighMode,
                .low_mode = LowMode,
                .filter_ns = FilterNs,
                .accum = Accum,
                .source = Source,
            },
            &state.unit,
            &state.chan));
        Init::pass(state.init);
    }
};

}  // namespace arc
