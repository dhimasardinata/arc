#pragma once

#include <cstddef>

#include "esp_err.h"
#include "esp_lcd_types.h"

struct esp_lcd_panel_t {
    int placeholder{};
};

inline esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t panel) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t panel) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_del(esp_lcd_panel_handle_t panel) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_draw_bitmap(
    esp_lcd_panel_handle_t panel,
    int,
    int,
    int,
    int,
    const void*) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_draw_bitmap_2d(
    esp_lcd_panel_handle_t panel,
    int,
    int,
    int,
    int,
    const void*,
    std::size_t,
    std::size_t,
    int,
    int,
    int,
    int) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_disp_on_off(esp_lcd_panel_handle_t panel, bool) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_disp_sleep(esp_lcd_panel_handle_t panel, bool) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_mirror(esp_lcd_panel_handle_t panel, bool, bool) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_swap_xy(esp_lcd_panel_handle_t panel, bool) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_set_gap(esp_lcd_panel_handle_t panel, int, int) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}

inline esp_err_t esp_lcd_panel_invert_color(esp_lcd_panel_handle_t panel, bool) noexcept
{
    return panel == nullptr ? ESP_ERR_INVALID_ARG : ESP_OK;
}
