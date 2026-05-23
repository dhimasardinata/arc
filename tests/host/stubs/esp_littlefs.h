#pragma once

#include <cstddef>

#include "esp_err.h"

struct esp_vfs_littlefs_conf_t {
    const char* base_path{};
    const char* partition_label{};
    bool format_if_mount_failed{};
    bool read_only{};
    bool dont_mount{};
};

inline bool esp_littlefs_stub_mounted = false;
inline std::size_t esp_littlefs_stub_total = 256U;
inline std::size_t esp_littlefs_stub_used = 16U;

inline esp_err_t esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t* conf)
{
    if (conf == nullptr || conf->base_path == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_littlefs_stub_mounted = !conf->dont_mount;
    return ESP_OK;
}

inline esp_err_t esp_vfs_littlefs_unregister(const char*)
{
    esp_littlefs_stub_mounted = false;
    return ESP_OK;
}

inline bool esp_littlefs_mounted(const char*)
{
    return esp_littlefs_stub_mounted;
}

inline esp_err_t esp_littlefs_info(const char*, std::size_t* total, std::size_t* used)
{
    if (total == nullptr || used == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *total = esp_littlefs_stub_total;
    *used = esp_littlefs_stub_used;
    return ESP_OK;
}

inline esp_err_t esp_littlefs_format(const char*)
{
    return ESP_OK;
}
