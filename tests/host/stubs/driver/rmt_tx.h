#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "esp_err.h"
#include "soc/gpio_num.h"

using rmt_channel_handle_t = void*;

struct rmt_tx_channel_config_t {
    gpio_num_t gpio_num{};
    rmt_clock_source_t clk_src{RMT_CLK_SRC_DEFAULT};
    std::uint32_t resolution_hz{};
    std::size_t mem_block_symbols{};
    std::size_t trans_queue_depth{};
    int intr_priority{};
    struct {
        std::uint32_t invert_out : 1 {};
        std::uint32_t with_dma : 1 {};
        std::uint32_t allow_pd : 1 {};
        std::uint32_t init_level : 1 {};
    } flags{};
};

struct rmt_transmit_config_t {
    int loop_count{};
    struct {
        std::uint32_t eot_level : 1 {};
        std::uint32_t queue_nonblocking : 1 {};
    } flags{};
};

struct rmt_carrier_config_t {
    std::uint32_t frequency_hz{};
    float duty_cycle{};
    struct {
        std::uint32_t polarity_active_low : 1 {};
        std::uint32_t always_on : 1 {};
    } flags{};
};

inline int arc_host_rmt_transmit_calls{};
inline std::size_t arc_host_rmt_last_bytes{};

inline esp_err_t rmt_new_tx_channel(
    const rmt_tx_channel_config_t* config,
    rmt_channel_handle_t* channel)
{
    if (config == nullptr || channel == nullptr || config->resolution_hz == 0U ||
        config->mem_block_symbols == 0U || config->trans_queue_depth == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    *channel = reinterpret_cast<rmt_channel_handle_t>(0x1000);
    return ESP_OK;
}

inline esp_err_t rmt_enable(const rmt_channel_handle_t channel)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t rmt_disable(const rmt_channel_handle_t channel)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t rmt_apply_carrier(
    const rmt_channel_handle_t channel,
    const rmt_carrier_config_t*)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t rmt_transmit(
    const rmt_channel_handle_t channel,
    const rmt_encoder_handle_t encoder,
    const void* symbols,
    const std::size_t bytes,
    const rmt_transmit_config_t* config)
{
    if (channel == nullptr || encoder == nullptr || symbols == nullptr || bytes == 0U || config == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    ++arc_host_rmt_transmit_calls;
    arc_host_rmt_last_bytes = bytes;
    return ESP_OK;
}

inline esp_err_t rmt_tx_wait_all_done(const rmt_channel_handle_t channel, int)
{
    return channel != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}
