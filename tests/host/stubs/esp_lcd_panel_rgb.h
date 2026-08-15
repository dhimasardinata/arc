#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_types.h"
#include "soc/gpio_num.h"

#define ESP_LCD_RGB_BUS_WIDTH_MAX 16
#define ESP_RGB_LCD_PANEL_MAX_FB_NUM 3

struct esp_lcd_rgb_timing_t {
    std::uint32_t pclk_hz{};
    std::uint32_t h_res{};
    std::uint32_t v_res{};
    std::uint32_t hsync_pulse_width{};
    std::uint32_t hsync_back_porch{};
    std::uint32_t hsync_front_porch{};
    std::uint32_t vsync_pulse_width{};
    std::uint32_t vsync_back_porch{};
    std::uint32_t vsync_front_porch{};
    struct {
        std::uint32_t hsync_idle_low : 1;
        std::uint32_t vsync_idle_low : 1;
        std::uint32_t de_idle_high : 1;
        std::uint32_t pclk_active_neg : 1;
        std::uint32_t pclk_idle_high : 1;
    } flags{};
};

struct esp_lcd_rgb_panel_config_t {
    lcd_clock_source_t clk_src{LCD_CLK_SRC_DEFAULT};
    esp_lcd_rgb_timing_t timings{};
    std::size_t data_width{};
    lcd_color_format_t in_color_format{LCD_COLOR_FMT_NONE};
    lcd_color_format_t out_color_format{LCD_COLOR_FMT_NONE};
    void* user_fbs[ESP_RGB_LCD_PANEL_MAX_FB_NUM]{};
    std::size_t bounce_buffer_size_px{};
    std::size_t dma_burst_size{};
    gpio_num_t data_gpio_nums[ESP_LCD_RGB_BUS_WIDTH_MAX]{};
    gpio_num_t hsync_gpio_num{GPIO_NUM_NC};
    gpio_num_t vsync_gpio_num{GPIO_NUM_NC};
    gpio_num_t de_gpio_num{GPIO_NUM_NC};
    gpio_num_t pclk_gpio_num{GPIO_NUM_NC};
    gpio_num_t disp_gpio_num{GPIO_NUM_NC};
    std::size_t num_fbs{};
    struct {
        std::uint32_t disp_active_low : 1;
        std::uint32_t refresh_on_demand : 1;
        std::uint32_t fb_in_psram : 1;
        std::uint32_t double_fb : 1;
        std::uint32_t no_fb : 1;
        std::uint32_t bb_invalidate_cache : 1;
    } flags{};
};

struct esp_lcd_rgb_panel_event_data_t {
    std::uint32_t event_id{};
};

using esp_lcd_rgb_panel_event_cb_t = bool (*)(
    esp_lcd_panel_handle_t,
    const esp_lcd_rgb_panel_event_data_t*,
    void*);

struct esp_lcd_rgb_panel_event_callbacks_t {
    esp_lcd_rgb_panel_event_cb_t on_color_trans_done{};
    esp_lcd_rgb_panel_event_cb_t on_frame_buf_complete{};
    esp_lcd_rgb_panel_event_cb_t on_vsync{};
};

inline esp_lcd_panel_t esp_lcd_rgb_panel_stub{};

inline esp_err_t esp_lcd_new_rgb_panel(
    const esp_lcd_rgb_panel_config_t* config,
    esp_lcd_panel_handle_t* out) noexcept
{
    if (config == nullptr || out == nullptr || config->data_width == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = &esp_lcd_rgb_panel_stub;
    return ESP_OK;
}

inline esp_err_t esp_lcd_rgb_panel_register_event_callbacks(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_rgb_panel_event_callbacks_t*,
    void*) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_rgb_panel_refresh(esp_lcd_panel_handle_t panel) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_rgb_panel_restart(esp_lcd_panel_handle_t panel) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_rgb_panel_set_pclk(esp_lcd_panel_handle_t panel, std::uint32_t) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_rgb_panel_set_yuv_conversion(
    esp_lcd_panel_handle_t panel,
    const esp_lcd_color_conv_yuv_config_t*) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline void* esp_lcd_rgb_alloc_draw_buffer(
    esp_lcd_panel_handle_t panel,
    const std::size_t bytes,
    const std::uint32_t caps) noexcept
{
    return panel == nullptr || bytes == 0U ? nullptr : heap_caps_malloc(bytes, caps);
}

inline esp_err_t esp_lcd_rgb_panel_get_frame_buffer(
    esp_lcd_panel_handle_t panel,
    const int count,
    void** fb0,
    void** fb1 = nullptr,
    void** fb2 = nullptr) noexcept
{
    static std::uint8_t frame0[16]{};
    static std::uint8_t frame1[16]{};
    static std::uint8_t frame2[16]{};

    if (panel == nullptr || count <= 0 || fb0 == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *fb0 = frame0;
    if (count > 1 && fb1 != nullptr) {
        *fb1 = frame1;
    }
    if (count > 2 && fb2 != nullptr) {
        *fb2 = frame2;
    }
    return ESP_OK;
}
