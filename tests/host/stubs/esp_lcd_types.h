#pragma once

#include <cstdint>

struct esp_lcd_panel_t;
using esp_lcd_panel_handle_t = esp_lcd_panel_t*;

enum lcd_clock_source_t : std::uint32_t {
    LCD_CLK_SRC_DEFAULT = 0,
};

enum lcd_color_format_t : std::uint32_t {
    LCD_COLOR_FMT_NONE = 0,
    LCD_COLOR_FMT_RGB565 = 1,
    LCD_COLOR_FMT_RGB888 = 2,
};

struct esp_lcd_color_conv_yuv_config_t {
    std::uint32_t yuv_range{};
};
