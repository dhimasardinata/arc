#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_check.h"
#include "hal/rmt_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin,
          std::uint32_t Hz,
          std::size_t Symbols = 64,
          std::size_t Depth = 1,
          bool Dma = false,
          rmt_clock_source_t Source = RMT_CLK_SRC_DEFAULT>
struct Burst {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid RMT pin");
    static_assert(Hz != 0U, "RMT resolution must be non-zero");
    static_assert(Symbols >= 2U && (Symbols % 2U) == 0U, "RMT symbol buffer must be even and non-zero");
    static_assert(Depth > 0U, "RMT queue depth must be non-zero");

    [[nodiscard]] static constexpr rmt_symbol_word_t symbol(
        const std::uint16_t d0,
        const bool l0,
        const std::uint16_t d1,
        const bool l1) noexcept
    {
        return rmt_symbol_word_t{
            .val =
                (static_cast<std::uint32_t>(d0 & 0x7fffU)) |
                (static_cast<std::uint32_t>(l0 ? 1U : 0U) << 15U) |
                (static_cast<std::uint32_t>(d1 & 0x7fffU) << 16U) |
                (static_cast<std::uint32_t>(l1 ? 1U : 0U) << 31U),
        };
    }

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

    [[nodiscard]] static esp_err_t send(
        const rmt_symbol_word_t* symbols,
        const std::size_t count,
        const int loop = 0,
        const bool nonblocking = true,
        const bool eot = false) noexcept
    {
        init();

        if (!state.enabled) {
            start();
        }

        rmt_transmit_config_t config{};
        config.loop_count = loop;
        config.flags.eot_level = eot ? 1U : 0U;
        config.flags.queue_nonblocking = nonblocking ? 1U : 0U;
        return rmt_transmit(
            state.channel,
            state.encoder,
            symbols,
            count * sizeof(rmt_symbol_word_t),
            &config);
    }

    template <std::size_t N>
    [[nodiscard]] static esp_err_t send(
        const rmt_symbol_word_t (&symbols)[N],
        const int loop = 0,
        const bool nonblocking = true,
        const bool eot = false) noexcept
    {
        return send(symbols, N, loop, nonblocking, eot);
    }

    template <std::size_t N>
    [[nodiscard]] static esp_err_t send(
        const std::array<rmt_symbol_word_t, N>& symbols,
        const int loop = 0,
        const bool nonblocking = true,
        const bool eot = false) noexcept
    {
        return send(symbols.data(), N, loop, nonblocking, eot);
    }

    [[nodiscard]] static esp_err_t wait(const int timeout_ms = -1) noexcept
    {
        init();
        return rmt_tx_wait_all_done(state.channel, timeout_ms);
    }

private:
    struct State {
        rmt_channel_handle_t channel{};
        rmt_encoder_handle_t encoder{};
        bool enabled{};
    };

    constinit static inline State state{};

    static void init()
    {
        if (state.channel != nullptr) {
            return;
        }

        rmt_tx_channel_config_t channel{};
        channel.gpio_num = static_cast<gpio_num_t>(Pin);
        channel.clk_src = Source;
        channel.resolution_hz = Hz;
        channel.mem_block_symbols = Symbols;
        channel.trans_queue_depth = Depth;
        channel.intr_priority = 0;
        channel.flags.invert_out = 0;
        channel.flags.with_dma = Dma ? 1U : 0U;
        channel.flags.allow_pd = 0;
        channel.flags.init_level = 0;
        ESP_ERROR_CHECK(rmt_new_tx_channel(&channel, &state.channel));

        rmt_copy_encoder_config_t encoder{};
        ESP_ERROR_CHECK(rmt_new_copy_encoder(&encoder, &state.encoder));
    }
};

}  // namespace arc
