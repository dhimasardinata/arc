#pragma once

#include "esp_err.h"
#include "nvs.h"

inline esp_err_t nvs_flash_init()
{
    return ESP_OK;
}

inline esp_err_t nvs_flash_erase()
{
    arc_host_nvs::reset();
    return ESP_OK;
}
