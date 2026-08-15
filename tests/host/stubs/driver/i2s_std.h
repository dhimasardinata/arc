#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "soc/gpio_num.h"

using i2s_chan_handle_t = void*;

enum i2s_port_t {
    I2S_NUM_AUTO = -1,
    I2S_NUM_0 = 0,
    I2S_NUM_1 = 1,
};

enum i2s_role_t {
    I2S_ROLE_MASTER = 0,
    I2S_ROLE_SLAVE = 1,
};

enum i2s_data_bit_width_t {
    I2S_DATA_BIT_WIDTH_8BIT = 8,
    I2S_DATA_BIT_WIDTH_16BIT = 16,
    I2S_DATA_BIT_WIDTH_24BIT = 24,
    I2S_DATA_BIT_WIDTH_32BIT = 32,
};

enum i2s_slot_mode_t {
    I2S_SLOT_MODE_MONO = 1,
    I2S_SLOT_MODE_STEREO = 2,
};

enum i2s_slot_bit_width_t {
    I2S_SLOT_BIT_WIDTH_AUTO = 0,
};

enum i2s_std_slot_mask_t {
    I2S_STD_SLOT_LEFT = 1,
    I2S_STD_SLOT_RIGHT = 2,
    I2S_STD_SLOT_BOTH = 3,
};

enum i2s_clock_src_t {
    I2S_CLK_SRC_DEFAULT = 0,
};

enum i2s_mclk_multiple_t {
    I2S_MCLK_MULTIPLE_256 = 256,
    I2S_MCLK_MULTIPLE_384 = 384,
};

inline constexpr gpio_num_t I2S_GPIO_UNUSED = GPIO_NUM_NC;

struct i2s_event_data_t {};

struct i2s_chan_config_t {
    int id{I2S_NUM_AUTO};
    i2s_role_t role{I2S_ROLE_MASTER};
    std::uint32_t dma_desc_num{};
    std::uint32_t dma_frame_num{};
    bool auto_clear_after_cb{};
    bool auto_clear_before_cb{};
    bool allow_pd{};
    int intr_priority{};
};

struct i2s_chan_info_t {
    int id{};
    i2s_role_t role{I2S_ROLE_MASTER};
};

struct i2s_std_slot_config_t {
    i2s_data_bit_width_t data_bit_width{I2S_DATA_BIT_WIDTH_16BIT};
    i2s_slot_bit_width_t slot_bit_width{I2S_SLOT_BIT_WIDTH_AUTO};
    i2s_slot_mode_t slot_mode{I2S_SLOT_MODE_STEREO};
    i2s_std_slot_mask_t slot_mask{I2S_STD_SLOT_BOTH};
    bool left_align{};
    bool big_endian{};
    bool bit_order_lsb{};
    std::uint32_t ws_width{};
    bool ws_pol{};
    bool bit_shift{};
};

struct i2s_std_clk_config_t {
    std::uint32_t sample_rate_hz{};
    i2s_clock_src_t clk_src{I2S_CLK_SRC_DEFAULT};
    std::uint32_t ext_clk_freq_hz{};
    i2s_mclk_multiple_t mclk_multiple{I2S_MCLK_MULTIPLE_256};
    std::uint32_t bclk_div{};
};

struct i2s_std_gpio_config_t {
    gpio_num_t mclk{GPIO_NUM_NC};
    gpio_num_t bclk{GPIO_NUM_NC};
    gpio_num_t ws{GPIO_NUM_NC};
    gpio_num_t dout{GPIO_NUM_NC};
    gpio_num_t din{GPIO_NUM_NC};
    struct {
        bool mclk_inv{};
        bool bclk_inv{};
        bool ws_inv{};
    } invert_flags{};
};

struct i2s_std_config_t {
    i2s_std_clk_config_t clk_cfg{};
    i2s_std_slot_config_t slot_cfg{};
    i2s_std_gpio_config_t gpio_cfg{};
};

using i2s_callback_t = bool (*)(i2s_chan_handle_t, i2s_event_data_t*, void*);

struct i2s_event_callbacks_t {
    i2s_callback_t on_sent{};
    i2s_callback_t on_send_q_ovf{};
    i2s_callback_t on_recv{};
    i2s_callback_t on_recv_q_ovf{};
};

[[nodiscard]] constexpr i2s_chan_config_t arc_host_i2s_channel_default_config(
    const int port,
    const i2s_role_t role) noexcept
{
    return i2s_chan_config_t{
        .id = port,
        .role = role,
        .dma_desc_num = 6U,
        .dma_frame_num = 240U,
    };
}

#define I2S_CHANNEL_DEFAULT_CONFIG(port, role) arc_host_i2s_channel_default_config((port), (role))

inline esp_err_t i2s_new_channel(
    const i2s_chan_config_t* const config,
    i2s_chan_handle_t* const tx,
    i2s_chan_handle_t* const rx)
{
    if (config == nullptr || config->dma_desc_num == 0U || config->dma_frame_num == 0U || (tx == nullptr && rx == nullptr)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (tx != nullptr) {
        *tx = reinterpret_cast<i2s_chan_handle_t>(0x3100);
    }
    if (rx != nullptr) {
        *rx = reinterpret_cast<i2s_chan_handle_t>(0x3200);
    }
    return ESP_OK;
}

inline esp_err_t i2s_del_channel(const i2s_chan_handle_t channel)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_init_std_mode(
    const i2s_chan_handle_t channel,
    const i2s_std_config_t* const config)
{
    return channel != nullptr && config != nullptr && config->clk_cfg.sample_rate_hz != 0U ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_register_event_callback(
    const i2s_chan_handle_t channel,
    const i2s_event_callbacks_t* const callbacks,
    void*)
{
    return channel != nullptr && callbacks != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_enable(const i2s_chan_handle_t channel)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_disable(const i2s_chan_handle_t channel)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_reconfig_std_clock(
    const i2s_chan_handle_t channel,
    const i2s_std_clk_config_t* const config)
{
    return channel != nullptr && config != nullptr && config->sample_rate_hz != 0U ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t i2s_channel_preload_data(
    const i2s_chan_handle_t channel,
    const void* const data,
    const std::size_t size,
    std::size_t* const loaded)
{
    if (channel == nullptr || loaded == nullptr || (data == nullptr && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    *loaded = size;
    return ESP_OK;
}

inline esp_err_t i2s_channel_write(
    const i2s_chan_handle_t channel,
    const void* const data,
    const std::size_t size,
    std::size_t* const wrote,
    std::uint32_t)
{
    if (channel == nullptr || wrote == nullptr || (data == nullptr && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    *wrote = size;
    return ESP_OK;
}

inline esp_err_t i2s_channel_read(
    const i2s_chan_handle_t channel,
    void* const data,
    const std::size_t size,
    std::size_t* const got,
    std::uint32_t)
{
    if (channel == nullptr || got == nullptr || (data == nullptr && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    *got = size;
    return ESP_OK;
}

inline esp_err_t i2s_channel_get_info(
    const i2s_chan_handle_t channel,
    i2s_chan_info_t* const info)
{
    if (channel == nullptr || info == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *info = i2s_chan_info_t{};
    return ESP_OK;
}
