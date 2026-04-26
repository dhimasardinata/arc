#pragma once

#include <concepts>
#include <cstdint>

#include "driver/gptimer.h"
#include "driver/gptimer_types.h"
#include "esp_attr.h"
#include "esp_check.h"

#include "arc/detail/cold.hpp"
#include "arc/init.hpp"

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
        gptimer_alarm_config_t config{};
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
        detail::cold::TimerAlarmCallback alarm{};
        bool enabled{};
        bool running{};
        bool bound{};
        std::uint32_t init{};
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

    [[gnu::cold]] static void create()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        ESP_ERROR_CHECK(detail::cold::timer_create({Hz, Source, Direction}, &state.timer));
        Init::pass(state.init);
    }

    template <typename Handler>
    static void ensure()
    {
        create();

        if constexpr (TimerAlarm<Handler>) {
            constexpr auto callback = detail::cold::TimerAlarmCallback{&on_alarm<Handler>};
            if (state.bound) {
                configASSERT(state.alarm == callback);
                return;
            }

            configASSERT(!state.enabled);
            ESP_ERROR_CHECK(detail::cold::timer_bind(state.timer, callback));
            state.alarm = callback;
            state.bound = true;
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
