#pragma once

#include <cstddef>

#include "esp_err.h"

struct esp_vfs_spiffs_conf_t {
    const char* base_path{};
    const char* partition_label{};
    std::size_t max_files{};
    bool format_if_mount_failed{};
};

inline int esp_spiffs_stub_registered = 0;
inline bool esp_spiffs_stub_mounted = false;
inline std::size_t esp_spiffs_stub_total = 64U;
inline std::size_t esp_spiffs_stub_used = 8U;

inline esp_err_t esp_vfs_spiffs_register(const esp_vfs_spiffs_conf_t* conf)
{
    if (conf == nullptr || conf->base_path == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    ++esp_spiffs_stub_registered;
    esp_spiffs_stub_mounted = true;
    return ESP_OK;
}

inline esp_err_t esp_vfs_spiffs_unregister(const char*)
{
    esp_spiffs_stub_mounted = false;
    return ESP_OK;
}

inline bool esp_spiffs_mounted(const char*)
{
    return esp_spiffs_stub_mounted;
}

inline esp_err_t esp_spiffs_info(const char*, std::size_t* total, std::size_t* used)
{
    if (total == nullptr || used == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *total = esp_spiffs_stub_total;
    *used = esp_spiffs_stub_used;
    return ESP_OK;
}

inline esp_err_t esp_spiffs_format(const char*)
{
    return ESP_OK;
}

inline esp_err_t esp_spiffs_check(const char*)
{
    return ESP_OK;
}

inline esp_err_t esp_spiffs_gc(const char*, std::size_t)
{
    return ESP_OK;
}
