#pragma once

#include "esp_err.h"

using rmt_encoder_handle_t = void*;

struct rmt_copy_encoder_config_t {};

inline esp_err_t rmt_new_copy_encoder(
    const rmt_copy_encoder_config_t* config,
    rmt_encoder_handle_t* encoder)
{
    if (config == nullptr || encoder == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *encoder = reinterpret_cast<rmt_encoder_handle_t>(0x2000);
    return ESP_OK;
}
