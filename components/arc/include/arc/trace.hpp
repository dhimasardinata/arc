#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "driver/rmt_common.h"
#include "driver/rmt_rx.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "hal/rmt_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/detail/cold.hpp"
#include "arc/fence.hpp"
#include "arc/init.hpp"

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
        state.used.store(0U, std::memory_order_relaxed);
        state.last.store(false, std::memory_order_relaxed);
        state.ready.store(false, std::memory_order_release);
        fence();
    }

    [[nodiscard]] static esp_err_t arm(
        const std::uint32_t min_ns = MinNs,
        const std::uint32_t max_ns = MaxNs,
        const bool partial = false) noexcept
    {
        start();
        clear();
        rmt_receive_config_t config{};
        config.signal_range_min_ns = min_ns;
        config.signal_range_max_ns = max_ns;
        config.flags.en_partial_rx = partial ? 1U : 0U;
        return rmt_receive(state.channel, buffer.data(), sizeof(buffer), &config);
    }

    [[nodiscard]] static bool ready() noexcept
    {
        return state.ready.load(std::memory_order_acquire);
    }

    [[nodiscard]] static bool last() noexcept
    {
        return state.last.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::size_t size() noexcept
    {
        return state.used.load(std::memory_order_relaxed);
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
        std::atomic<std::size_t> used{0U};
        std::atomic<bool> ready{false};
        std::atomic<bool> last{false};
        bool enabled{};
        bool bound{};
        std::uint32_t init{};
    };

    constinit static inline State state{};
    constinit static inline std::array<rmt_symbol_word_t, Symbols> buffer{};

    IRAM_ATTR static bool on_done(
        rmt_channel_handle_t,
        const rmt_rx_done_event_data_t* event,
        void*) noexcept
    {
        if (event == nullptr) {
            state.used.store(0U, std::memory_order_relaxed);
            state.last.store(true, std::memory_order_relaxed);
        } else {
            state.used.store(event->num_symbols, std::memory_order_relaxed);
            state.last.store(event->flags.is_last != 0U, std::memory_order_relaxed);
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

        ESP_ERROR_CHECK(detail::cold::trace_create({Pin, Hz, Symbols, Dma, Source}, &state.channel));

        if (!state.bound) {
            ESP_ERROR_CHECK(detail::cold::trace_bind(state.channel, &on_done));
            state.bound = true;
        }
        Init::pass(state.init);
    }
};

}  // namespace arc
