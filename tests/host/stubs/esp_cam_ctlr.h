#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "hal/color_types.h"

using esp_cam_ctlr_handle_t = void*;

enum cam_ctlr_color_t {
    CAM_CTLR_COLOR_RGB565 = 0,
};

struct esp_cam_ctlr_trans_t {
    void* buffer{};
    std::size_t buflen{};
    std::size_t received_size{};
};

struct cam_ctlr_format_conv_config_t {
    cam_ctlr_color_t src_format{CAM_CTLR_COLOR_RGB565};
    cam_ctlr_color_t dst_format{CAM_CTLR_COLOR_RGB565};
    color_conv_std_rgb_yuv_t conv_std{COLOR_CONV_STD_RGB_YUV_BT601};
    std::uint32_t data_width{};
    color_range_t input_range{COLOR_RANGE_FULL};
    color_range_t output_range{COLOR_RANGE_FULL};
};

inline esp_err_t esp_cam_ctlr_enable(const esp_cam_ctlr_handle_t handle)
{
    return handle != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_cam_ctlr_start(const esp_cam_ctlr_handle_t handle)
{
    return handle != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_cam_ctlr_stop(const esp_cam_ctlr_handle_t handle)
{
    return handle != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline void* esp_cam_ctlr_alloc_buffer(
    const esp_cam_ctlr_handle_t handle,
    const std::size_t bytes,
    const std::uint32_t caps)
{
    return handle != nullptr && bytes != 0U ? heap_caps_malloc(bytes, caps) : nullptr;
}

inline esp_err_t esp_cam_ctlr_receive(
    const esp_cam_ctlr_handle_t handle,
    esp_cam_ctlr_trans_t* const trans,
    std::uint32_t)
{
    if (handle == nullptr || trans == nullptr || trans->buffer == nullptr || trans->buflen == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    trans->received_size = trans->buflen;
    return ESP_OK;
}

inline esp_err_t esp_cam_ctlr_format_conversion(
    const esp_cam_ctlr_handle_t handle,
    const cam_ctlr_format_conv_config_t* const config)
{
    return handle != nullptr && config != nullptr && config->data_width != 0U ? ESP_OK : ESP_ERR_INVALID_ARG;
}
