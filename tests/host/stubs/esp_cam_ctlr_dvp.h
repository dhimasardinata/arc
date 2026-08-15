#pragma once

#include <cstdint>

#include "esp_cam_ctlr.h"
#include "esp_err.h"
#include "hal/color_types.h"
#include "soc/gpio_num.h"

#define ESP_CAM_CTLR_DVP_DATA_SIG_NUM 16

enum cam_ctlr_data_width_t {
    CAM_CTLR_DATA_WIDTH_8 = 8,
    CAM_CTLR_DATA_WIDTH_10 = 10,
    CAM_CTLR_DATA_WIDTH_12 = 12,
    CAM_CTLR_DATA_WIDTH_16 = 16,
};

enum cam_clock_source_t {
    CAM_CLK_SRC_DEFAULT = 0,
};

struct esp_cam_ctlr_dvp_pin_config_t {
    cam_ctlr_data_width_t data_width{CAM_CTLR_DATA_WIDTH_8};
    gpio_num_t vsync_io{GPIO_NUM_NC};
    gpio_num_t de_io{GPIO_NUM_NC};
    gpio_num_t pclk_io{GPIO_NUM_NC};
    gpio_num_t xclk_io{GPIO_NUM_NC};
    gpio_num_t data_io[ESP_CAM_CTLR_DVP_DATA_SIG_NUM]{};
};

struct esp_cam_ctlr_dvp_config_t {
    int ctlr_id{};
    cam_clock_source_t clk_src{CAM_CLK_SRC_DEFAULT};
    std::uint32_t h_res{};
    std::uint32_t v_res{};
    cam_ctlr_color_t input_data_color_type{CAM_CTLR_COLOR_RGB565};
    cam_ctlr_color_t output_data_color_type{CAM_CTLR_COLOR_RGB565};
    color_conv_std_rgb_yuv_t conv_std{COLOR_CONV_STD_RGB_YUV_BT601};
    color_range_t input_range{COLOR_RANGE_FULL};
    color_range_t output_range{COLOR_RANGE_FULL};
    std::uint32_t cam_data_width{};
    std::uint32_t bit_swap_en : 1 {};
    std::uint32_t byte_swap_en : 1 {};
    std::uint32_t bk_buffer_dis : 1 {};
    std::uint32_t pin_dont_init : 1 {};
    std::uint32_t pic_format_jpeg : 1 {};
    std::uint32_t external_xtal : 1 {};
    std::uint32_t dma_burst_size{};
    std::uint32_t xclk_freq{};
    const esp_cam_ctlr_dvp_pin_config_t* pin{};
};

inline esp_err_t esp_cam_new_dvp_ctlr(
    const esp_cam_ctlr_dvp_config_t* const config,
    esp_cam_ctlr_handle_t* const out)
{
    if (config == nullptr || out == nullptr || config->pin == nullptr || config->h_res == 0U || config->v_res == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = reinterpret_cast<esp_cam_ctlr_handle_t>(0x5100);
    return ESP_OK;
}
