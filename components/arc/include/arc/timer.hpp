#pragma once

#include <concepts>
#include <cstdint>

#include "driver/gptimer.h"
#include "driver/gptimer_types.h"
#include "esp_attr.h"
#include "esp_check.h"

namespace arc {

template <typename Handler>
concept TimerAlarm = requires(const gptimer_alarm_event_data_t& event) {
    { Handler::alarm(event) } -> std::same_as<bool>;
};

template <std::uint32_t Hz,
          gptimer_clock_source_t Source = GPTIMER_CLK_SRC_DEFAULT,
          gptimer_count_direction_t Direction = GPTIMER_COUNT_UP>
struct Timer {
    static_assert(Hz != 0U, "timer resolution must be non-zero");

    static void init()
    {
        create();
    }

    static void start()
    {
        ensure<void>();
        run();
    }

    template <typename Handler>
    static void start()
    {
        ensure<Handler>();
        run();
    }

    static void stop()
    {
        if (state.timer == nullptr || !state.running) {
            return;
        }

        ESP_ERROR_CHECK(gptimer_stop(state.timer));
        state.running = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        run();
    }

    static void enable()
    {
        ensure<void>();

        if (!state.enabled) {
            ESP_ERROR_CHECK(gptimer_enable(state.timer));
            state.enabled = true;
        }
    }

    static void zero(const std::uint64_t value = 0) noexcept
    {
        create();
        ESP_ERROR_CHECK(gptimer_set_raw_count(state.timer, value));
    }

    [[nodiscard]] static std::uint64_t read() noexcept
    {
        create();
        std::uint64_t value = 0;
        ESP_ERROR_CHECK(gptimer_get_raw_count(state.timer, &value));
        return value;
    }

    [[nodiscard]] static std::uint32_t hz() noexcept
    {
        if (state.timer == nullptr) {
            return Hz;
        }

        std::uint32_t value = 0;
        ESP_ERROR_CHECK(gptimer_get_resolution(state.timer, &value));
        return value;
    }

    static void alarm(
        const std::uint64_t count,
        const std::uint64_t reload = 0,
        const bool auto_reload = false) noexcept
    {
        create();
        static gptimer_alarm_config_t config{};

        config.alarm_count = count;
        config.reload_count = reload;
        config.flags.auto_reload_on_alarm = auto_reload ? 1U : 0U;
        ESP_ERROR_CHECK(gptimer_set_alarm_action(state.timer, &config));
    }

    static void off() noexcept
    {
        create();
        ESP_ERROR_CHECK(gptimer_set_alarm_action(state.timer, nullptr));
    }

    [[nodiscard]] static gptimer_handle_t native()
    {
        create();
        return state.timer;
    }

private:
    struct State {
        gptimer_handle_t timer{};
        bool enabled{};
        bool running{};
        bool bound{};
    };

    constinit static inline State state{};

    template <typename Handler>
    IRAM_ATTR static bool on_alarm(
        gptimer_handle_t,
        const gptimer_alarm_event_data_t* event,
        void*) noexcept
    {
        if constexpr (TimerAlarm<Handler>) {
            return event != nullptr ? Handler::alarm(*event) : false;
        } else {
            return false;
        }
    }

    static void create()
    {
        if (state.timer != nullptr) {
            return;
        }

        gptimer_config_t config{};
        config.clk_src = Source;
        config.direction = Direction;
        config.resolution_hz = Hz;
        config.intr_priority = 0;
        config.flags.intr_shared = 0;
        config.flags.allow_pd = 0;
        ESP_ERROR_CHECK(gptimer_new_timer(&config, &state.timer));
    }

    template <typename Handler>
    static void ensure()
    {
        create();

        if constexpr (TimerAlarm<Handler>) {
            if (!state.bound && !state.enabled) {
                gptimer_event_callbacks_t cbs{};
                cbs.on_alarm = &on_alarm<Handler>;
                ESP_ERROR_CHECK(gptimer_register_event_callbacks(state.timer, &cbs, nullptr));
                state.bound = true;
            }
        }
    }

    static void run()
    {
        enable();

        if (!state.running) {
            ESP_ERROR_CHECK(gptimer_start(state.timer));
            state.running = true;
        }
    }
};

}  // namespace arc
