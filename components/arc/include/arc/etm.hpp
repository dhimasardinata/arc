#pragma once

#include <concepts>
#include <cstdio>

#include "driver/gpio.h"
#include "driver/gpio_etm.h"
#include "driver/gptimer_etm.h"
#include "driver/gptimer_types.h"
#include "esp_check.h"
#include "esp_etm.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

static_assert(SOC_ETM_SUPPORTED, "arc::Route requires ETM support");
static_assert(SOC_TIMER_SUPPORT_ETM, "arc::Route requires GPTimer ETM support");

template <typename Event>
concept EtmEvent = requires {
    { Event::event() } -> std::same_as<esp_etm_event_handle_t>;
};

template <typename Task>
concept EtmTask = requires {
    { Task::task() } -> std::same_as<esp_etm_task_handle_t>;
};

struct Etm {
    static void dump(FILE* out = stdout) noexcept
    {
        ESP_ERROR_CHECK(esp_etm_dump(out));
    }
};

template <typename Event, typename Task, bool AllowPd = false>
struct Route {
    static_assert(EtmEvent<Event>, "route event must expose event()");
    static_assert(EtmTask<Task>, "route task must expose task()");

    static void boot()
    {
        init();
    }

    static void on()
    {
        init();

        if (!state.enabled) {
            ESP_ERROR_CHECK(esp_etm_channel_enable(state.chan));
            state.enabled = true;
        }
    }

    static void off()
    {
        if (state.chan == nullptr || !state.enabled) {
            return;
        }

        ESP_ERROR_CHECK(esp_etm_channel_disable(state.chan));
        state.enabled = false;
    }

private:
    struct State {
        esp_etm_channel_handle_t chan{};
        bool enabled{};
    };

    constinit static inline State state{};

    static void init()
    {
        Event::boot();
        Task::boot();

        if (state.chan != nullptr) {
            return;
        }

        esp_etm_channel_config_t config{};
        config.flags.allow_pd = AllowPd ? 1U : 0U;
        ESP_ERROR_CHECK(esp_etm_new_channel(&config, &state.chan));
        ESP_ERROR_CHECK(esp_etm_channel_connect(state.chan, Event::event(), Task::task()));
    }
};

template <int Pin, gpio_etm_event_edge_t Edge = GPIO_ETM_EVENT_EDGE_ANY>
struct GpioEdge {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid ETM input pin");

    static void in()
    {
        boot();
    }

    static void boot()
    {
        if (state.event != nullptr) {
            return;
        }

        gpio_config_t config{};
        config.pin_bit_mask = 1ULL << Pin;
        config.mode = GPIO_MODE_INPUT;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&config));

        gpio_etm_event_config_t etm{};
        etm.edge = Edge;
        ESP_ERROR_CHECK(gpio_new_etm_event(&etm, &state.event));
        ESP_ERROR_CHECK(gpio_etm_event_bind_gpio(state.event, Pin));
    }

    [[nodiscard]] static esp_etm_event_handle_t event()
    {
        boot();
        return state.event;
    }

private:
    struct State {
        esp_etm_event_handle_t event{};
    };

    constinit static inline State state{};
};

template <int Pin, gpio_etm_task_action_t Action = GPIO_ETM_TASK_ACTION_TOG>
struct GpioTask {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid ETM output pin");

    static void out()
    {
        boot();
    }

    static void boot()
    {
        if (state.task != nullptr) {
            return;
        }

        gpio_config_t config{};
        config.pin_bit_mask = 1ULL << Pin;
        config.mode = GPIO_MODE_OUTPUT;
        config.pull_up_en = GPIO_PULLUP_DISABLE;
        config.pull_down_en = GPIO_PULLDOWN_DISABLE;
        config.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&config));

        gpio_etm_task_config_t etm{};
        etm.action = Action;
        ESP_ERROR_CHECK(gpio_new_etm_task(&etm, &state.task));
        ESP_ERROR_CHECK(gpio_etm_task_add_gpio(state.task, Pin));
    }

    [[nodiscard]] static esp_etm_task_handle_t task()
    {
        boot();
        return state.task;
    }

private:
    struct State {
        esp_etm_task_handle_t task{};
    };

    constinit static inline State state{};
};

template <typename Clock, gptimer_etm_event_type_t Type = GPTIMER_ETM_EVENT_ALARM_MATCH>
struct TimerEvent {
    static void boot()
    {
        if (state.event != nullptr) {
            return;
        }

        gptimer_etm_event_config_t config{};
        config.event_type = Type;
        ESP_ERROR_CHECK(gptimer_new_etm_event(Clock::native(), &config, &state.event));
    }

    [[nodiscard]] static esp_etm_event_handle_t event()
    {
        boot();
        return state.event;
    }

private:
    struct State {
        esp_etm_event_handle_t event{};
    };

    constinit static inline State state{};
};

template <typename Clock, gptimer_etm_task_type_t Type>
struct TimerTask {
    static void boot()
    {
        if (state.task != nullptr) {
            return;
        }

        gptimer_etm_task_config_t config{};
        config.task_type = Type;
        ESP_ERROR_CHECK(gptimer_new_etm_task(Clock::native(), &config, &state.task));
    }

    [[nodiscard]] static esp_etm_task_handle_t task()
    {
        boot();
        return state.task;
    }

private:
    struct State {
        esp_etm_task_handle_t task{};
    };

    constinit static inline State state{};
};

template <int Pin>
using Rise = GpioEdge<Pin, GPIO_ETM_EVENT_EDGE_POS>;

template <int Pin>
using Fall = GpioEdge<Pin, GPIO_ETM_EVENT_EDGE_NEG>;

template <int Pin>
using AnyEdge = GpioEdge<Pin, GPIO_ETM_EVENT_EDGE_ANY>;

template <int Pin>
using Set = GpioTask<Pin, GPIO_ETM_TASK_ACTION_SET>;

template <int Pin>
using Clear = GpioTask<Pin, GPIO_ETM_TASK_ACTION_CLR>;

template <int Pin>
using Toggle = GpioTask<Pin, GPIO_ETM_TASK_ACTION_TOG>;

template <typename Clock>
using Alarm = TimerEvent<Clock, GPTIMER_ETM_EVENT_ALARM_MATCH>;

template <typename Clock>
using Start = TimerTask<Clock, GPTIMER_ETM_TASK_START_COUNT>;

template <typename Clock>
using Stop = TimerTask<Clock, GPTIMER_ETM_TASK_STOP_COUNT>;

template <typename Clock>
using Arm = TimerTask<Clock, GPTIMER_ETM_TASK_EN_ALARM>;

template <typename Clock>
using Reload = TimerTask<Clock, GPTIMER_ETM_TASK_RELOAD>;

}  // namespace arc
