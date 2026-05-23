#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "wear_levelling.h"

struct esp_vfs_fat_mount_config_t {
    bool format_if_mount_failed{};
    int max_files{};
    std::size_t allocation_unit_size{};
};

#define VFS_FAT_MOUNT_DEFAULT_CONFIG() \
    esp_vfs_fat_mount_config_t {}

inline bool esp_fat_stub_mounted = false;
inline std::uint64_t esp_fat_stub_total = 128U;
inline std::uint64_t esp_fat_stub_free = 32U;

inline esp_err_t esp_vfs_fat_spiflash_mount_rw_wl(
    const char* base,
    const char*,
    const esp_vfs_fat_mount_config_t*,
    wl_handle_t* handle)
{
    if (base == nullptr || handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *handle = 7;
    esp_fat_stub_mounted = true;
    return ESP_OK;
}

inline esp_err_t esp_vfs_fat_spiflash_mount_ro(
    const char* base,
    const char*,
    const esp_vfs_fat_mount_config_t*)
{
    if (base == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_fat_stub_mounted = true;
    return ESP_OK;
}

inline esp_err_t esp_vfs_fat_spiflash_unmount_rw_wl(const char*, wl_handle_t)
{
    esp_fat_stub_mounted = false;
    return ESP_OK;
}

inline esp_err_t esp_vfs_fat_spiflash_unmount_ro(const char*, const char*)
{
    esp_fat_stub_mounted = false;
    return ESP_OK;
}

inline esp_err_t esp_vfs_fat_info(const char*, std::uint64_t* total, std::uint64_t* free)
{
    if (total == nullptr || free == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *total = esp_fat_stub_total;
    *free = esp_fat_stub_free;
    return ESP_OK;
}

inline esp_err_t esp_vfs_fat_spiflash_format_rw_wl(const char*, const char*)
{
    return ESP_OK;
}
