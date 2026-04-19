#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/fence.hpp"
#include "driver/rmt_common.h"
#include "driver/rmt_rx.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "hal/rmt_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin,
          std::uint32_t Hz,
          std::size_t Symbols = 64,
          std::uint32_t MinNs = 100,
          std::uint32_t MaxNs = 200000,
          bool Dma = false,
          rmt_clock_source_t Source = RMT_CLK_SRC_DEFAULT>
struct Trace {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid RMT pin");
    static_assert(Hz != 0U, "RMT resolution must be non-zero");
    static_assert(Symbols >= 2U && (Symbols % 2U) == 0U, "RMT symbol buffer must be even and non-zero");
    static_assert(MinNs < MaxNs, "RMT min signal must be lower than max signal");

    using View = std::span<const rmt_symbol_word_t>;

    static void start()
    {
        init();

        if (!state.enabled) {
            ESP_ERROR_CHECK(rmt_enable(state.channel));
            state.enabled = true;
        }
    }

    static void stop()
    {
        if (!state.enabled) {
            return;
        }

        ESP_ERROR_CHECK(rmt_disable(state.channel));
        state.enabled = false;
    }

    [[nodiscard]] static esp_err_t carrier(
        const std::uint32_t frequency_hz,
        const float duty_cycle = 50.0f,
        const bool active_low = false,
        const bool always_on = false) noexcept
    {
        init();

        rmt_carrier_config_t config{};
        config.frequency_hz = frequency_hz;
        config.duty_cycle = duty_cycle;
        config.flags.polarity_active_low = active_low ? 1U : 0U;
        config.flags.always_on = always_on ? 1U : 0U;
        return rmt_apply_carrier(state.channel, &config);
    }

    [[nodiscard]] static esp_err_t plain() noexcept
    {
        init();
        return rmt_apply_carrier(state.channel, nullptr);
    }

    static void clear() noexcept
    {
        state.ready = false;
        state.used = 0;
        state.last = false;
        fence();
    }

    [[nodiscard]] static esp_err_t arm(
        const std::uint32_t min_ns = MinNs,
        const std::uint32_t max_ns = MaxNs,
        const bool partial = false) noexcept
    {
        start();
        clear();

        static rmt_receive_config_t config{};
        config.signal_range_min_ns = min_ns;
        config.signal_range_max_ns = max_ns;
        config.flags.en_partial_rx = partial ? 1U : 0U;

        return rmt_receive(state.channel, buffer.data(), sizeof(buffer), &config);
    }

    [[nodiscard]] static bool ready() noexcept
    {
        fence();
        return state.ready;
    }

    [[nodiscard]] static bool last() noexcept
    {
        fence();
        return state.last;
    }

    [[nodiscard]] static std::size_t size() noexcept
    {
        fence();
        return state.used;
    }

    [[nodiscard]] static const rmt_symbol_word_t* data() noexcept
    {
        return buffer.data();
    }

    [[nodiscard]] static View view() noexcept
    {
        return View{buffer.data(), size()};
    }

private:
    struct State {
        rmt_channel_handle_t channel{};
        volatile std::size_t used{};
        volatile bool ready{};
        volatile bool last{};
        bool enabled{};
        bool bound{};
    };

    constinit static inline State state{};
    constinit static inline std::array<rmt_symbol_word_t, Symbols> buffer{};

    IRAM_ATTR static bool on_done(
        rmt_channel_handle_t,
        const rmt_rx_done_event_data_t* event,
        void*) noexcept
    {
        if (event == nullptr) {
            state.used = 0;
            state.last = true;
        } else {
            state.used = event->num_symbols;
            state.last = event->flags.is_last != 0U;
        }

        fence();
        state.ready = true;
        return false;
    }

    static void init()
    {
        if (state.channel != nullptr) {
            return;
        }

        rmt_rx_channel_config_t channel{};
        channel.gpio_num = static_cast<gpio_num_t>(Pin);
        channel.clk_src = Source;
        channel.resolution_hz = Hz;
        channel.mem_block_symbols = Symbols;
        channel.intr_priority = 0;
        channel.flags.invert_in = 0;
        channel.flags.with_dma = Dma ? 1U : 0U;
        channel.flags.allow_pd = 0;
        ESP_ERROR_CHECK(rmt_new_rx_channel(&channel, &state.channel));

        if (!state.bound) {
            rmt_rx_event_callbacks_t cbs{};
            cbs.on_recv_done = &on_done;
            ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(state.channel, &cbs, nullptr));
            state.bound = true;
        }
    }
};

}  // namespace arc
