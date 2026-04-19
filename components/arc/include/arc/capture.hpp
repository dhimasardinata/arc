#pragma once

#include <cstdint>

#include "driver/mcpwm_prelude.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/fence.hpp"

namespace arc {

template <int Pin,
          std::uint32_t Hz = 1'000'000,
          int Group = 0,
          std::uint32_t Prescale = 1,
          bool Rise = true,
          bool Fall = true,
          bool Invert = false,
          mcpwm_capture_clock_source_t Source = MCPWM_CAPTURE_CLK_SRC_DEFAULT>
struct Capture {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid MCPWM capture pin");
    static_assert(Group >= 0, "invalid MCPWM capture group");
    static_assert(Hz != 0U, "capture resolution must be non-zero");
    static_assert(Prescale != 0U, "capture prescale must be non-zero");
    static_assert(Rise || Fall, "capture must latch at least one edge");

    static void start()
    {
        init();

        if (!state.enabled) {
            ESP_ERROR_CHECK(mcpwm_capture_channel_enable(state.chan));
            state.enabled = true;
        }

        if (!state.running) {
            ESP_ERROR_CHECK(mcpwm_capture_timer_enable(state.timer));
            ESP_ERROR_CHECK(mcpwm_capture_timer_start(state.timer));
            state.running = true;
        }
    }

    static void stop()
    {
        if (!state.running) {
            return;
        }

        ESP_ERROR_CHECK(mcpwm_capture_timer_stop(state.timer));
        ESP_ERROR_CHECK(mcpwm_capture_timer_disable(state.timer));
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

    static void clear() noexcept
    {
        state.stamp = 0U;
        state.rise = false;
        state.ready = false;
        state.edges = 0U;
        state.rise_stamp = 0U;
        state.fall_stamp = 0U;
        state.period = 0U;
        state.high = 0U;
        state.low = 0U;
        state.have_rise = false;
        state.have_fall = false;
        fence();
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static bool ready() noexcept
    {
        fence();
        return state.ready;
    }

    static void flush() noexcept
    {
        fence();
        state.ready = false;
    }

    [[nodiscard]] static std::uint32_t ticks() noexcept
    {
        fence();
        return state.stamp;
    }

    [[nodiscard]] static std::uint32_t edges() noexcept
    {
        fence();
        return state.edges;
    }

    [[nodiscard]] static bool rising() noexcept
    {
        fence();
        return state.rise;
    }

    [[nodiscard]] static std::uint32_t period() noexcept
    {
        fence();
        return state.period;
    }

    [[nodiscard]] static std::uint32_t high() noexcept
    {
        fence();
        return state.high;
    }

    [[nodiscard]] static std::uint32_t low() noexcept
    {
        fence();
        return state.low;
    }

    [[nodiscard]] static std::uint32_t latched() noexcept
    {
        init();
        std::uint32_t value = 0U;
        ESP_ERROR_CHECK(mcpwm_capture_get_latched_value(state.chan, &value));
        return value;
    }

    static void soft() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_capture_channel_trigger_soft_catch(state.chan));
    }

private:
    struct State {
        mcpwm_cap_timer_handle_t timer{};
        mcpwm_cap_channel_handle_t chan{};
        volatile std::uint32_t stamp{};
        volatile bool rise{};
        volatile bool ready{};
        volatile std::uint32_t edges{};
        volatile std::uint32_t rise_stamp{};
        volatile std::uint32_t fall_stamp{};
        volatile std::uint32_t period{};
        volatile std::uint32_t high{};
        volatile std::uint32_t low{};
        volatile bool have_rise{};
        volatile bool have_fall{};
        bool enabled{};
        bool running{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_edge(
        mcpwm_cap_channel_handle_t,
        const mcpwm_capture_event_data_t* event,
        void*) noexcept
    {
        if (event == nullptr) {
            return false;
        }

        const auto tick = event->cap_value;
        const bool rise = event->cap_edge == MCPWM_CAP_EDGE_POS;

        state.stamp = tick;
        state.rise = rise;
        state.edges += 1U;

        if (rise) {
            if (state.have_rise) {
                state.period = tick - state.rise_stamp;
            }
            if (state.have_fall) {
                state.low = tick - state.fall_stamp;
            }
            state.rise_stamp = tick;
            state.have_rise = true;
        } else {
            if (state.have_rise) {
                state.high = tick - state.rise_stamp;
            }
            state.fall_stamp = tick;
            state.have_fall = true;
        }

        fence();
        state.ready = true;
        return false;
    }

    static void init()
    {
        if (state.timer != nullptr) {
            return;
        }

        mcpwm_capture_timer_config_t timer{};
        timer.group_id = Group;
        timer.clk_src = Source;
        timer.resolution_hz = Hz;
        timer.flags.allow_pd = 0U;
        ESP_ERROR_CHECK(mcpwm_new_capture_timer(&timer, &state.timer));

        mcpwm_capture_channel_config_t chan{};
        chan.gpio_num = Pin;
        chan.intr_priority = 0;
        chan.prescale = Prescale;
        chan.flags.pos_edge = Rise ? 1U : 0U;
        chan.flags.neg_edge = Fall ? 1U : 0U;
        chan.flags.invert_cap_signal = Invert ? 1U : 0U;
        ESP_ERROR_CHECK(mcpwm_new_capture_channel(state.timer, &chan, &state.chan));

        mcpwm_capture_event_callbacks_t cbs{};
        cbs.on_cap = &on_edge;
        ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(state.chan, &cbs, nullptr));
    }
};

}  // namespace arc
