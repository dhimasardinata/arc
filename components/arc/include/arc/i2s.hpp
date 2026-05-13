#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "driver/i2s_std.h"
#if __has_include("driver/i2s_pdm.h")
#include "driver/i2s_pdm.h"
#endif
#if __has_include("driver/i2s_tdm.h")
#include "driver/i2s_tdm.h"
#endif
#include "esp_attr.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"
#include "arc/result.hpp"
#include "arc/sdk.hpp"

namespace arc {

enum class I2sStd : std::uint8_t {
    philips,
    msb,
    pcm,
};

enum class I2sTdmFmt : std::uint8_t {
    philips,
    msb,
    pcm_short,
    pcm_long,
};

#if __has_include("driver/i2s_pdm.h")
enum class I2sPdmData : std::uint8_t {
    pcm,
    raw,
};

template <i2s_slot_mode_t Slots>
[[nodiscard]] consteval std::uint32_t i2s_pdm_mask_default() noexcept
{
    return Slots == I2S_SLOT_MODE_MONO
        ? static_cast<std::uint32_t>(I2S_PDM_SLOT_LEFT)
        : static_cast<std::uint32_t>(I2S_PDM_SLOT_BOTH);
}
#endif

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

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(Port, Role);
        chan.dma_desc_num = Desc;
        chan.dma_frame_num = Frames;
        chan.auto_clear_after_cb = false;
        chan.auto_clear_before_cb = false;
        chan.allow_pd = false;
        chan.intr_priority = 0;

        if constexpr (kTx && kRx) {
            const auto err = i2s_new_channel(&chan, &state.tx, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else if constexpr (kTx) {
            const auto err = i2s_new_channel(&chan, &state.tx, nullptr);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else {
            const auto err = i2s_new_channel(&chan, nullptr, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        }

        auto std = config(Hz);
        if constexpr (kTx) {
            const auto err = i2s_channel_init_std_mode(state.tx, &std);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_tx();
        }
        if constexpr (kRx) {
            const auto err = i2s_channel_init_std_mode(state.rx, &std);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_rx();
        }

        state.rate = Hz;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
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
    [[nodiscard]] static Result<std::size_t> preload(
        const void* const src,
        const std::size_t size) noexcept
        requires(Enabled)
    {
        std::size_t loaded{};
        const auto err = preload(src, size, &loaded);
        if (err != ESP_OK) {
            return fail(err);
        }
        return loaded;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> preload(const std::span<T, Extent> src) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return preload(src.data(), src.size_bytes());
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

    template <bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const void* const src,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t wrote{};
        const auto err = write(src, size, &wrote, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return wrote;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const std::span<T, Extent> src,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return write(src.data(), src.size_bytes(), timeout_ms);
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

    template <bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        void* const dst,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t got{};
        const auto err = read(dst, size, &got, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return got;
    }

    template <typename T, std::size_t Extent, bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        const std::span<T, Extent> dst,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && !std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    {
        return read(dst.data(), dst.size_bytes(), timeout_ms);
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
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv() noexcept
    {
        return __atomic_load_n(&state.recv, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t send_ovf() noexcept
    {
        return __atomic_load_n(&state.send_ovf, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv_ovf() noexcept
    {
        return __atomic_load_n(&state.recv_ovf, __ATOMIC_ACQUIRE);
    }

private:
    struct State {
        i2s_chan_handle_t tx{};
        i2s_chan_handle_t rx{};
        std::uint32_t rate{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t recv{};
        alignas(cache_line) std::uint32_t send_ovf{};
        alignas(cache_line) std::uint32_t recv_ovf{};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_sent(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_send_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.send_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    [[nodiscard]] static constexpr i2s_std_slot_config_t slot() noexcept
    {
        i2s_std_slot_config_t cfg{};
        cfg.data_bit_width = Bits;
        cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        cfg.slot_mode = Slots;
        cfg.slot_mask = I2S_STD_SLOT_BOTH;
        cfg.left_align = true;
        cfg.big_endian = false;
        cfg.bit_order_lsb = false;

        if constexpr (Format == I2sStd::pcm) {
            cfg.ws_width = 1U;
            cfg.ws_pol = true;
            cfg.bit_shift = true;
        } else if constexpr (Format == I2sStd::msb) {
            cfg.ws_width = static_cast<std::uint32_t>(Bits);
            cfg.ws_pol = false;
            cfg.bit_shift = false;
        } else {
            cfg.ws_width = static_cast<std::uint32_t>(Bits);
            cfg.ws_pol = false;
            cfg.bit_shift = true;
        }

        return cfg;
    }

    [[nodiscard]] static constexpr i2s_std_clk_config_t clock(const std::uint32_t rate) noexcept
    {
        i2s_std_clk_config_t cfg{};
        cfg.sample_rate_hz = rate;
        cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        cfg.ext_clk_freq_hz = 0U;
        cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        cfg.bclk_div = 8U;
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

    static void clear() noexcept
    {
        if constexpr (kTx) {
            if (state.tx != nullptr) {
                (void)i2s_del_channel(state.tx);
                state.tx = nullptr;
            }
        }
        if constexpr (kRx) {
            if (state.rx != nullptr) {
                (void)i2s_del_channel(state.rx);
                state.rx = nullptr;
            }
        }
    }
};

#if __has_include("driver/i2s_tdm.h") && SOC_I2S_SUPPORTS_TDM
template <int Bclk,
          int Ws,
          int Dout = static_cast<int>(I2S_GPIO_UNUSED),
          int Din = static_cast<int>(I2S_GPIO_UNUSED),
          std::uint32_t Hz = 48'000,
          i2s_data_bit_width_t Bits = I2S_DATA_BIT_WIDTH_16BIT,
          i2s_slot_mode_t Slots = I2S_SLOT_MODE_STEREO,
          std::uint32_t Mask = static_cast<std::uint32_t>(I2S_TDM_SLOT0) | static_cast<std::uint32_t>(I2S_TDM_SLOT1),
          I2sTdmFmt Format = I2sTdmFmt::philips,
          std::uint32_t Total = I2S_TDM_AUTO_SLOT_NUM,
          int Port = I2S_NUM_AUTO,
          std::uint32_t Desc = 6,
          std::uint32_t Frames = 240,
          int Mclk = static_cast<int>(I2S_GPIO_UNUSED),
          i2s_role_t Role = I2S_ROLE_MASTER>
struct I2sTdm {
    static constexpr bool kTx = Dout >= 0 && Dout < SOC_GPIO_PIN_COUNT;
    static constexpr bool kRx = Din >= 0 && Din < SOC_GPIO_PIN_COUNT;

    [[nodiscard]] static consteval std::uint32_t span_slots() noexcept
    {
        std::uint32_t mask = Mask;
        std::uint32_t span = 0U;
        while (mask != 0U) {
            ++span;
            mask >>= 1U;
        }
        return span;
    }

    [[nodiscard]] static consteval std::uint32_t total_slots() noexcept
    {
        return Total == I2S_TDM_AUTO_SLOT_NUM ? span_slots() : Total;
    }

    [[nodiscard]] static consteval i2s_mclk_multiple_t mclk_multiple() noexcept
    {
        if constexpr (total_slots() > 2U) {
            return I2S_MCLK_MULTIPLE_512;
        } else if constexpr (Bits == I2S_DATA_BIT_WIDTH_24BIT) {
            return I2S_MCLK_MULTIPLE_384;
        } else {
            return I2S_MCLK_MULTIPLE_256;
        }
    }

    static_assert(Bclk >= 0 && Bclk < SOC_GPIO_PIN_COUNT, "invalid I2S TDM BCLK pin");
    static_assert(Ws >= 0 && Ws < SOC_GPIO_PIN_COUNT, "invalid I2S TDM WS pin");
    static_assert(kTx || Dout == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S TDM DOUT pin");
    static_assert(kRx || Din == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S TDM DIN pin");
    static_assert(kTx || kRx, "I2S TDM lane must expose TX, RX, or both");
    static_assert(Hz != 0U, "I2S TDM sample rate must be non-zero");
    static_assert(Desc > 0U, "I2S TDM DMA descriptor count must be non-zero");
    static_assert(Frames > 0U, "I2S TDM DMA frame count must be non-zero");
    static_assert(Mask != 0U, "I2S TDM slot mask must not be zero");
    static_assert((Mask & ~0xFFFFU) == 0U, "I2S TDM slot mask exceeds 16 slots");
    static_assert(Total == I2S_TDM_AUTO_SLOT_NUM || Total <= 16U, "I2S TDM total slots must not exceed 16");
    static_assert(Total == I2S_TDM_AUTO_SLOT_NUM || Total >= span_slots(), "I2S TDM total slots must cover the highest active slot");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(Port, Role);
        chan.dma_desc_num = Desc;
        chan.dma_frame_num = Frames;
        chan.auto_clear_after_cb = false;
        chan.auto_clear_before_cb = false;
        chan.allow_pd = false;
        chan.intr_priority = 0;

        if constexpr (kTx && kRx) {
            const auto err = i2s_new_channel(&chan, &state.tx, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else if constexpr (kTx) {
            const auto err = i2s_new_channel(&chan, &state.tx, nullptr);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else {
            const auto err = i2s_new_channel(&chan, nullptr, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        }

        auto tdm = config(Hz);
        if constexpr (kTx) {
            const auto err = i2s_channel_init_tdm_mode(state.tx, &tdm);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_tx();
        }
        if constexpr (kRx) {
            const auto err = i2s_channel_init_tdm_mode(state.rx, &tdm);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_rx();
        }

        state.rate = Hz;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
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
            ESP_ERROR_CHECK(i2s_channel_reconfig_tdm_clock(state.tx, &clk));
        }
        if constexpr (kRx) {
            ESP_ERROR_CHECK(i2s_channel_reconfig_tdm_clock(state.rx, &clk));
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
    [[nodiscard]] static Result<std::size_t> preload(
        const void* const src,
        const std::size_t size) noexcept
        requires(Enabled)
    {
        std::size_t loaded{};
        const auto err = preload(src, size, &loaded);
        if (err != ESP_OK) {
            return fail(err);
        }
        return loaded;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> preload(const std::span<T, Extent> src) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return preload(src.data(), src.size_bytes());
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

    template <bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const void* const src,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t wrote{};
        const auto err = write(src, size, &wrote, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return wrote;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const std::span<T, Extent> src,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return write(src.data(), src.size_bytes(), timeout_ms);
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

    template <bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        void* const dst,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t got{};
        const auto err = read(dst, size, &got, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return got;
    }

    template <typename T, std::size_t Extent, bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        const std::span<T, Extent> dst,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && !std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    {
        return read(dst.data(), dst.size_bytes(), timeout_ms);
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

    [[nodiscard]] static constexpr std::uint32_t mask() noexcept
    {
        return Mask;
    }

    [[nodiscard]] static constexpr std::uint32_t slots() noexcept
    {
        return total_slots();
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv() noexcept
    {
        return __atomic_load_n(&state.recv, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t send_ovf() noexcept
    {
        return __atomic_load_n(&state.send_ovf, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv_ovf() noexcept
    {
        return __atomic_load_n(&state.recv_ovf, __ATOMIC_ACQUIRE);
    }

private:
    struct State {
        i2s_chan_handle_t tx{};
        i2s_chan_handle_t rx{};
        std::uint32_t rate{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t recv{};
        alignas(cache_line) std::uint32_t send_ovf{};
        alignas(cache_line) std::uint32_t recv_ovf{};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_sent(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_send_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.send_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    [[nodiscard]] static constexpr i2s_tdm_slot_config_t slot() noexcept
    {
        i2s_tdm_slot_config_t cfg{};
        cfg.data_bit_width = Bits;
        cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        cfg.slot_mode = Slots;
        cfg.slot_mask = static_cast<i2s_tdm_slot_mask_t>(Mask);
        cfg.left_align = false;
        cfg.big_endian = false;
        cfg.bit_order_lsb = false;
        cfg.skip_mask = false;
        cfg.total_slot = total_slots();

        if constexpr (Format == I2sTdmFmt::pcm_short) {
            cfg.ws_width = 1U;
            cfg.ws_pol = true;
            cfg.bit_shift = true;
        } else if constexpr (Format == I2sTdmFmt::pcm_long) {
            cfg.ws_width = static_cast<std::uint32_t>(Bits);
            cfg.ws_pol = true;
            cfg.bit_shift = true;
        } else if constexpr (Format == I2sTdmFmt::msb) {
            cfg.ws_width = I2S_TDM_AUTO_WS_WIDTH;
            cfg.ws_pol = false;
            cfg.bit_shift = false;
        } else {
            cfg.ws_width = I2S_TDM_AUTO_WS_WIDTH;
            cfg.ws_pol = false;
            cfg.bit_shift = true;
        }

        return cfg;
    }

    [[nodiscard]] static constexpr i2s_tdm_clk_config_t clock(const std::uint32_t rate) noexcept
    {
        i2s_tdm_clk_config_t cfg{};
        cfg.sample_rate_hz = rate;
        cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        cfg.ext_clk_freq_hz = 0U;
        cfg.mclk_multiple = mclk_multiple();
        cfg.bclk_div = 8U;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_tdm_gpio_config_t gpio() noexcept
    {
        i2s_tdm_gpio_config_t cfg{};
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

    [[nodiscard]] static constexpr i2s_tdm_config_t config(const std::uint32_t rate) noexcept
    {
        i2s_tdm_config_t cfg{};
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

    static void clear() noexcept
    {
        if constexpr (kTx) {
            if (state.tx != nullptr) {
                (void)i2s_del_channel(state.tx);
                state.tx = nullptr;
            }
        }
        if constexpr (kRx) {
            if (state.rx != nullptr) {
                (void)i2s_del_channel(state.rx);
                state.rx = nullptr;
            }
        }
    }
};
#endif

#if __has_include("driver/i2s_pdm.h") && (SOC_I2S_SUPPORTS_PDM_TX || SOC_I2S_SUPPORTS_PDM_RX)
template <int Clk,
          int Dout = static_cast<int>(I2S_GPIO_UNUSED),
          int Din = static_cast<int>(I2S_GPIO_UNUSED),
          std::uint32_t Hz = 16'000,
          i2s_slot_mode_t Slots = I2S_SLOT_MODE_MONO,
          I2sPdmData Format = I2sPdmData::pcm,
          std::uint32_t Mask = i2s_pdm_mask_default<Slots>(),
          int Port = 0,
          std::uint32_t Desc = 6,
          std::uint32_t Frames = 240,
          i2s_role_t Role = I2S_ROLE_MASTER>
struct I2sPdm {
    static constexpr bool kTx = Dout >= 0 && Dout < SOC_GPIO_PIN_COUNT;
    static constexpr bool kRx = Din >= 0 && Din < SOC_GPIO_PIN_COUNT;

    static_assert(Clk >= 0 && Clk < SOC_GPIO_PIN_COUNT, "invalid I2S PDM CLK pin");
    static_assert(kTx || Dout == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S PDM DOUT pin");
    static_assert(kRx || Din == static_cast<int>(I2S_GPIO_UNUSED), "invalid I2S PDM DIN pin");
    static_assert(kTx || kRx, "I2S PDM lane must expose TX, RX, or both");
    static_assert(Hz != 0U, "I2S PDM sample rate must be non-zero");
    static_assert(Desc > 0U, "I2S PDM DMA descriptor count must be non-zero");
    static_assert(Frames > 0U, "I2S PDM DMA frame count must be non-zero");
    static_assert((Mask & ~0xFFU) == 0U, "I2S PDM slot mask exceeds supported lines");
    static_assert(Port == 0, "ESP32-S3 PDM is only available on I2S0");
#if SOC_I2S_SUPPORTS_PDM_TX
    static_assert(!kTx || Format == I2sPdmData::raw || SOC_I2S_SUPPORTS_PCM2PDM, "PCM PDM TX requires hardware PCM2PDM");
#else
    static_assert(!kTx, "I2S PDM TX is not supported on this target");
#endif
#if SOC_I2S_SUPPORTS_PDM_RX
    static_assert(!kRx || Format == I2sPdmData::raw || SOC_I2S_SUPPORTS_PDM2PCM, "PCM PDM RX requires hardware PDM2PCM");
#else
    static_assert(!kRx, "I2S PDM RX is not supported on this target");
#endif

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(Port, Role);
        chan.dma_desc_num = Desc;
        chan.dma_frame_num = Frames;
        chan.auto_clear_after_cb = false;
        chan.auto_clear_before_cb = false;
        chan.allow_pd = false;
        chan.intr_priority = 0;

        if constexpr (kTx && kRx) {
            const auto err = i2s_new_channel(&chan, &state.tx, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else if constexpr (kTx) {
            const auto err = i2s_new_channel(&chan, &state.tx, nullptr);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        } else {
            const auto err = i2s_new_channel(&chan, nullptr, &state.rx);
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        }

        if constexpr (kTx) {
            auto tx = tx_config(Hz);
            const auto err = i2s_channel_init_pdm_tx_mode(state.tx, &tx);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_tx();
        }
        if constexpr (kRx) {
            auto rx = rx_config(Hz);
            const auto err = i2s_channel_init_pdm_rx_mode(state.rx, &rx);
            if (err != ESP_OK) {
                clear();
                Init::fail(state.init);
                return err;
            }
            bind_rx();
        }

        state.rate = Hz;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
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

        if constexpr (kTx) {
            auto clk = tx_clock(value);
            ESP_ERROR_CHECK(i2s_channel_reconfig_pdm_tx_clock(state.tx, &clk));
        }
        if constexpr (kRx) {
            auto clk = rx_clock(value);
            ESP_ERROR_CHECK(i2s_channel_reconfig_pdm_rx_clock(state.rx, &clk));
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
    [[nodiscard]] static Result<std::size_t> preload(
        const void* const src,
        const std::size_t size) noexcept
        requires(Enabled)
    {
        std::size_t loaded{};
        const auto err = preload(src, size, &loaded);
        if (err != ESP_OK) {
            return fail(err);
        }
        return loaded;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> preload(const std::span<T, Extent> src) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return preload(src.data(), src.size_bytes());
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

    template <bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const void* const src,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t wrote{};
        const auto err = write(src, size, &wrote, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return wrote;
    }

    template <typename T, std::size_t Extent, bool Enabled = kTx>
    [[nodiscard]] static Result<std::size_t> write(
        const std::span<T, Extent> src,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && std::is_trivially_copyable_v<std::remove_cv_t<T>>)
    {
        return write(src.data(), src.size_bytes(), timeout_ms);
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

    template <bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        void* const dst,
        const std::size_t size,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled)
    {
        std::size_t got{};
        const auto err = read(dst, size, &got, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return got;
    }

    template <typename T, std::size_t Extent, bool Enabled = kRx>
    [[nodiscard]] static Result<std::size_t> read(
        const std::span<T, Extent> dst,
        const std::uint32_t timeout_ms = 1000U) noexcept
        requires(Enabled && !std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    {
        return read(dst.data(), dst.size_bytes(), timeout_ms);
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

    [[nodiscard]] static constexpr std::uint32_t mask() noexcept
    {
        return Mask;
    }

    [[nodiscard]] static constexpr I2sPdmData data() noexcept
    {
        return Format;
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv() noexcept
    {
        return __atomic_load_n(&state.recv, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t send_ovf() noexcept
    {
        return __atomic_load_n(&state.send_ovf, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t recv_ovf() noexcept
    {
        return __atomic_load_n(&state.recv_ovf, __ATOMIC_ACQUIRE);
    }

private:
    struct State {
        i2s_chan_handle_t tx{};
        i2s_chan_handle_t rx{};
        std::uint32_t rate{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t recv{};
        alignas(cache_line) std::uint32_t send_ovf{};
        alignas(cache_line) std::uint32_t recv_ovf{};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static consteval i2s_pdm_data_fmt_t fmt() noexcept
    {
        return Format == I2sPdmData::raw ? I2S_PDM_DATA_FMT_RAW : I2S_PDM_DATA_FMT_PCM;
    }

    IRAM_ATTR static bool on_sent(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_send_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.send_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_recv_ovf(i2s_chan_handle_t, i2s_event_data_t*, void*) noexcept
    {
        __atomic_add_fetch(&state.recv_ovf, 1U, __ATOMIC_RELEASE);
        return false;
    }

    [[nodiscard]] static constexpr i2s_pdm_tx_clk_config_t tx_clock(const std::uint32_t rate) noexcept
    {
        i2s_pdm_tx_clk_config_t cfg{};
        cfg.sample_rate_hz = rate;
        cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        cfg.up_sample_fp = 960U;
        cfg.up_sample_fs = 480U;
        cfg.bclk_div = 8U;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_rx_clk_config_t rx_clock(const std::uint32_t rate) noexcept
    {
        i2s_pdm_rx_clk_config_t cfg{};
        cfg.sample_rate_hz = rate;
        cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
        cfg.dn_sample_mode = I2S_PDM_DSR_8S;
        cfg.bclk_div = 8U;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_tx_slot_config_t tx_slot() noexcept
    {
        i2s_pdm_tx_slot_config_t cfg{};
        cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
        cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        cfg.slot_mode = Slots;
#if SOC_I2S_HW_VERSION_1
        cfg.slot_mask = static_cast<i2s_pdm_slot_mask_t>(Mask);
#endif
        cfg.data_fmt = fmt();
        cfg.sd_prescale = 0U;
        cfg.sd_scale = I2S_PDM_SIG_SCALING_MUL_1;
#if SOC_I2S_HW_VERSION_2
        cfg.hp_scale = I2S_PDM_SIG_SCALING_DIV_2;
        cfg.lp_scale = I2S_PDM_SIG_SCALING_MUL_1;
        cfg.sinc_scale = I2S_PDM_SIG_SCALING_MUL_1;
        cfg.line_mode = I2S_PDM_TX_ONE_LINE_CODEC;
        cfg.hp_en = Format == I2sPdmData::pcm;
        cfg.hp_cut_off_freq_hz = 35.5f;
        cfg.sd_dither = 0U;
        cfg.sd_dither2 = 1U;
#else
        cfg.hp_scale = I2S_PDM_SIG_SCALING_MUL_1;
        cfg.lp_scale = I2S_PDM_SIG_SCALING_MUL_1;
        cfg.sinc_scale = I2S_PDM_SIG_SCALING_MUL_1;
#endif
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_rx_slot_config_t rx_slot() noexcept
    {
        i2s_pdm_rx_slot_config_t cfg{};
        cfg.data_bit_width = I2S_DATA_BIT_WIDTH_16BIT;
        cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO;
        cfg.slot_mode = Slots;
        cfg.slot_mask = static_cast<i2s_pdm_slot_mask_t>(Mask);
        cfg.data_fmt = fmt();
#if SOC_I2S_SUPPORTS_PDM_RX_HP_FILTER
        cfg.hp_en = Format == I2sPdmData::pcm;
        cfg.hp_cut_off_freq_hz = 35.5f;
        cfg.amplify_num = 1U;
#endif
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_tx_gpio_config_t tx_gpio() noexcept
    {
        i2s_pdm_tx_gpio_config_t cfg{};
        cfg.clk = static_cast<gpio_num_t>(Clk);
        cfg.dout = static_cast<gpio_num_t>(Dout);
#if SOC_I2S_PDM_MAX_TX_LINES > 1
        cfg.dout2 = I2S_GPIO_UNUSED;
#endif
        cfg.invert_flags.clk_inv = false;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_rx_gpio_config_t rx_gpio() noexcept
    {
        i2s_pdm_rx_gpio_config_t cfg{};
        cfg.clk = static_cast<gpio_num_t>(Clk);
        cfg.din = static_cast<gpio_num_t>(Din);
        cfg.invert_flags.clk_inv = false;
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_tx_config_t tx_config(const std::uint32_t rate) noexcept
    {
        i2s_pdm_tx_config_t cfg{};
        cfg.clk_cfg = tx_clock(rate);
        cfg.slot_cfg = tx_slot();
        cfg.gpio_cfg = tx_gpio();
        return cfg;
    }

    [[nodiscard]] static constexpr i2s_pdm_rx_config_t rx_config(const std::uint32_t rate) noexcept
    {
        i2s_pdm_rx_config_t cfg{};
        cfg.clk_cfg = rx_clock(rate);
        cfg.slot_cfg = rx_slot();
        cfg.gpio_cfg = rx_gpio();
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

    static void clear() noexcept
    {
        if constexpr (kTx) {
            if (state.tx != nullptr) {
                (void)i2s_del_channel(state.tx);
                state.tx = nullptr;
            }
        }
        if constexpr (kRx) {
            if (state.rx != nullptr) {
                (void)i2s_del_channel(state.rx);
                state.rx = nullptr;
            }
        }
    }
};
#endif

}  // namespace arc
