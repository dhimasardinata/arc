#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/i2s_std.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

enum class I2sStd : std::uint8_t {
    philips,
    msb,
    pcm,
};

template <int Bclk,
          int Ws,
          int Dout = static_cast<int>(I2S_GPIO_UNUSED),
          int Din = static_cast<int>(I2S_GPIO_UNUSED),
          std::uint32_t Hz = 48'000,
          i2s_data_bit_width_t Bits = I2S_DATA_BIT_WIDTH_16BIT,
          i2s_slot_mode_t Slots = I2S_SLOT_MODE_STEREO,
          I2sStd Format = I2sStd::philips,
          int Port = I2S_NUM_AUTO,
          std::uint32_t Desc = 6,
          std::uint32_t Frames = 240,
          int Mclk = static_cast<int>(I2S_GPIO_UNUSED),
          i2s_role_t Role = I2S_ROLE_MASTER>
struct I2s {
    static constexpr bool kTx = Dout >= 0 && Dout < SOC_GPIO_PIN_COUNT;
    static constexpr bool kRx = Din >= 0 && Din < SOC_GPIO_PIN_COUNT;

    static_assert(Bclk >= 0 && Bclk < SOC_GPIO_PIN_COUNT, "invalid I2S BCLK pin");
    static_assert(Ws >= 0 && Ws < SOC_GPIO_PIN_COUNT, "invalid I2S WS pin");
    static_assert(kTx || Dout == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S DOUT pin");
    static_assert(kRx || Din == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S DIN pin");
    static_assert(kTx || kRx, "I2S lane must expose TX, RX, or both");
    static_assert(Hz != 0U, "I2S sample rate must be non-zero");
    static_assert(Desc > 0U, "I2S DMA descriptor count must be non-zero");
    static_assert(Frames > 0U, "I2S DMA frame count must be non-zero");

    static void boot()
    {
        if (state.booted) {
            return;
        }

        i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(Port, Role);
        chan.dma_desc_num = Desc;
        chan.dma_frame_num = Frames;
        chan.auto_clear_after_cb = false;
        chan.auto_clear_before_cb = false;
        chan.allow_pd = false;
        chan.intr_priority = 0;

        if constexpr (kTx && kRx) {
            ESP_ERROR_CHECK(i2s_new_channel(&chan, &state.tx, &state.rx));
        } else if constexpr (kTx) {
            ESP_ERROR_CHECK(i2s_new_channel(&chan, &state.tx, nullptr));
        } else {
            ESP_ERROR_CHECK(i2s_new_channel(&chan, nullptr, &state.rx));
        }

        auto std = config(Hz);
        if constexpr (kTx) {
            ESP_ERROR_CHECK(i2s_channel_init_std_mode(state.tx, &std));
            bind_tx();
        }
        if constexpr (kRx) {
            ESP_ERROR_CHECK(i2s_channel_init_std_mode(state.rx, &std));
            bind_rx();
        }

        state.rate = Hz;
        state.booted = true;
    }

    static void start()
    {
        boot();

        if (state.running) {
            return;
        }

        if constexpr (kTx) {
            ESP_ERROR_CHECK(i2s_channel_enable(state.tx));
        }
        if constexpr (kRx) {
            ESP_ERROR_CHECK(i2s_channel_enable(state.rx));
        }

        state.running = true;
    }

    static void stop()
    {
        if (!state.running) {
            return;
        }

        if constexpr (kTx) {
            ESP_ERROR_CHECK(i2s_channel_disable(state.tx));
        }
        if constexpr (kRx) {
            ESP_ERROR_CHECK(i2s_channel_disable(state.rx));
        }

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

    static void rate(const std::uint32_t value)
    {
        boot();
        const bool was_running = state.running;
        if (was_running) {
            stop();
        }

        auto clk = clock(value);
        if constexpr (kTx) {
            ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(state.tx, &clk));
        }
        if constexpr (kRx) {
            ESP_ERROR_CHECK(i2s_channel_reconfig_std_clock(state.rx, &clk));
        }
        state.rate = value;

        if (was_running) {
            start();
        }
    }

    template <bool Enabled = kTx>
    [[nodiscard]] static esp_err_t preload(
        const void* const src,
        const std::size_t size,
        std::size_t* const loaded) noexcept
        requires(Enabled)
    {
        boot();
        return i2s_channel_preload_data(state.tx, src, size, loaded);
    }

    template <bool Enabled = kTx>
    [[nodiscard]] static esp_err_t write(
        const void* const src,
        const std::size_t size,
        std::size_t* const wrote,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        boot();
        return i2s_channel_write(state.tx, src, size, wrote, timeout_ms);
    }

    template <bool Enabled = kRx>
    [[nodiscard]] static esp_err_t read(
        void* const dst,
        const std::size_t size,
        std::size_t* const got,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        boot();
        return i2s_channel_read(state.rx, dst, size, got, timeout_ms);
    }

    template <bool Enabled = kTx>
    [[nodiscard]] static i2s_chan_info_t tx_info() noexcept
        requires(Enabled)
    {
        boot();
        i2s_chan_info_t info{};
        ESP_ERROR_CHECK(i2s_channel_get_info(state.tx, &info));
        return info;
    }

    template <bool Enabled = kRx>
    [[nodiscard]] static i2s_chan_info_t rx_info() noexcept
        requires(Enabled)
    {
        boot();
        i2s_chan_info_t info{};
        ESP_ERROR_CHECK(i2s_channel_get_info(state.rx, &info));
        return info;
    }

    [[nodiscard]] static constexpr bool duplex() noexcept
    {
        return kTx && kRx;
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static std::uint32_t rate() noexcept
    {
        boot();
        return state.rate;
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return state.sent;
    }

    [[nodiscard]] static std::uint32_t recv() noexcept
    {
        return state.recv;
    }

    [[nodiscard]] static std::uint32_t send_ovf() noexcept
    {
        return state.send_ovf;
    }

    [[nodiscard]] static std::uint32_t recv_ovf() noexcept
    {
        return state.recv_ovf;
    }

private:
    struct State {
        i2s_chan_handle_t tx{};
        i2s_chan_handle_t rx{};
        std::uint32_t rate{};
        volatile std::uint32_t sent{};
        volatile std::uint32_t recv{};
        volatile std::uint32_t send_ovf{};
        volatile std::uint32_t recv_ovf{};
        bool booted{};
        bool running{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_sent(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        state.sent += 1U;
        return false;
    }

    IRAM_ATTR static bool on_send_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        state.send_ovf += 1U;
        return false;
    }

    IRAM_ATTR static bool on_recv(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        state.recv += 1U;
        return false;
    }

    IRAM_ATTR static bool on_recv_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        state.recv_ovf += 1U;
        return false;
    }

    [[nodiscard]] static constexpr i2s_std_slot_config_t slot() noexcept
    {
        if constexpr (Format == I2sStd::philips) {
            return I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(Bits, Slots);
        } else if constexpr (Format == I2sStd::pcm) {
            return I2S_STD_PCM_SLOT_DEFAULT_CONFIG(Bits, Slots);
        } else {
            return I2S_STD_MSB_SLOT_DEFAULT_CONFIG(Bits, Slots);
        }
    }

    [[nodiscard]] static constexpr i2s_std_clk_config_t clock(const std::uint32_t rate) noexcept
    {
        auto cfg = i2s_std_clk_config_t{I2S_STD_CLK_DEFAULT_CONFIG(rate)};
        if constexpr (Bits == I2S_DATA_BIT_WIDTH_24BIT) {
            cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
        }
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_std_gpio_config_t gpio() noexcept
    {
        i2s_std_gpio_config_t cfg{};
        cfg.mclk = static_cast<gpio_num_t>(Mclk);
        cfg.bclk = static_cast<gpio_num_t>(Bclk);
        cfg.ws = static_cast<gpio_num_t>(Ws);
        cfg.dout = kTx ? static_cast<gpio_num_t>(Dout) : I2S_GPIO_UNUSED;
        cfg.din = kRx ? static_cast<gpio_num_t>(Din) : I2S_GPIO_UNUSED;
        cfg.invert_flags.mclk_inv = false;
        cfg.invert_flags.bclk_inv = false;
        cfg.invert_flags.ws_inv = false;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_std_config_t config(const std::uint32_t rate) noexcept
    {
        i2s_std_config_t cfg{};
        cfg.clk_cfg = clock(rate);
        cfg.slot_cfg = slot();
        cfg.gpio_cfg = gpio();
        return cfg;
    }

    static void bind_tx()
    {
        i2s_event_callbacks_t cb{};
        cb.on_sent = &on_sent;
        cb.on_send_q_ovf = &on_send_ovf;
        ESP_ERROR_CHECK(i2s_channel_register_event_callback(state.tx, &cb, nullptr));
    }

    static void bind_rx()
    {
        i2s_event_callbacks_t cb{};
        cb.on_recv = &on_recv;
        cb.on_recv_q_ovf = &on_recv_ovf;
        ESP_ERROR_CHECK(i2s_channel_register_event_callback(state.rx, &cb, nullptr));
    }
};

}  // namespace arc
