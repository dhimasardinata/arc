#pragma once

#include <atomic>
#include <cstdint>

#include "driver/mcpwm_prelude.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/detail/cold.hpp"
#include "arc/fence.hpp"
#include "arc/init.hpp"

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
        state.stamp.store(0U, std::memory_order_relaxed);
        state.rise.store(false, std::memory_order_relaxed);
        state.ready.store(false, std::memory_order_release);
        state.edges.store(0U, std::memory_order_relaxed);
        state.rise_stamp.store(0U, std::memory_order_relaxed);
        state.fall_stamp.store(0U, std::memory_order_relaxed);
        state.period.store(0U, std::memory_order_relaxed);
        state.high.store(0U, std::memory_order_relaxed);
        state.low.store(0U, std::memory_order_relaxed);
        state.have_rise.store(false, std::memory_order_relaxed);
        state.have_fall.store(false, std::memory_order_relaxed);
        fence();
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static bool ready() noexcept
    {
        return state.ready.load(std::memory_order_acquire);
    }

    static void flush() noexcept
    {
        state.ready.store(false, std::memory_order_release);
    }

    [[nodiscard]] static std::uint32_t ticks() noexcept
    {
        return state.stamp.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t edges() noexcept
    {
        return state.edges.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static bool rising() noexcept
    {
        return state.rise.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t period() noexcept
    {
        return state.period.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t high() noexcept
    {
        return state.high.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t low() noexcept
    {
        return state.low.load(std::memory_order_relaxed);
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
        std::atomic<std::uint32_t> stamp{0U};
        std::atomic<bool> rise{false};
        std::atomic<bool> ready{false};
        std::atomic<std::uint32_t> edges{0U};
        std::atomic<std::uint32_t> rise_stamp{0U};
        std::atomic<std::uint32_t> fall_stamp{0U};
        std::atomic<std::uint32_t> period{0U};
        std::atomic<std::uint32_t> high{0U};
        std::atomic<std::uint32_t> low{0U};
        std::atomic<bool> have_rise{false};
        std::atomic<bool> have_fall{false};
        bool enabled{};
        bool running{};
        std::uint32_t init{};
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

        state.stamp.store(tick, std::memory_order_relaxed);
        state.rise.store(rise, std::memory_order_relaxed);
        state.edges.fetch_add(1U, std::memory_order_relaxed);

        if (rise) {
            if (state.have_rise.load(std::memory_order_relaxed)) {
                state.period.store(
                    tick - state.rise_stamp.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
            }
            if (state.have_fall.load(std::memory_order_relaxed)) {
                state.low.store(
                    tick - state.fall_stamp.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
            }
            state.rise_stamp.store(tick, std::memory_order_relaxed);
            state.have_rise.store(true, std::memory_order_relaxed);
        } else {
            if (state.have_rise.load(std::memory_order_relaxed)) {
                state.high.store(
                    tick - state.rise_stamp.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
            }
            state.fall_stamp.store(tick, std::memory_order_relaxed);
            state.have_fall.store(true, std::memory_order_relaxed);
        }

        fence();
        state.ready.store(true, std::memory_order_release);
        return false;
    }

    [[gnu::cold]] static void init()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        ESP_ERROR_CHECK(detail::cold::capture_create(
            {Pin, Hz, Group, Prescale, Rise, Fall, Invert, Source},
            &state.timer,
            &state.chan));
        ESP_ERROR_CHECK(detail::cold::capture_bind(state.chan, &on_edge));
        Init::pass(state.init);
    }
};

}  // namespace arc
