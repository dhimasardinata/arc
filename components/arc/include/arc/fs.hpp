#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "wear_levelling.h"

namespace arc {

struct Fs {
    struct SpiffsInfo {
        std::size_t total{};
        std::size_t used{};
    };

    struct FatInfo {
        std::uint64_t total{};
        std::uint64_t free{};
    };

    [[nodiscard]] static esp_err_t spiffs(
        const char* const base = "/fs",
        const char* const label = nullptr,
        const std::size_t max_files = 4U,
        const bool format = false) noexcept
    {
        esp_vfs_spiffs_conf_t cfg{};
        cfg.base_path = base;
        cfg.partition_label = label;
        cfg.max_files = max_files;
        cfg.format_if_mount_failed = format;
        return esp_vfs_spiffs_register(&cfg);
    }

    [[nodiscard]] static esp_err_t spiffs_off(const char* const label = nullptr) noexcept
    {
        return esp_vfs_spiffs_unregister(label);
    }

    [[nodiscard]] static bool spiffs_on(const char* const label = nullptr) noexcept
    {
        return esp_spiffs_mounted(label);
    }

    [[nodiscard]] static esp_err_t spiffs_info(
        SpiffsInfo& out,
        const char* const label = nullptr) noexcept
    {
        return esp_spiffs_info(label, &out.total, &out.used);
    }

    [[nodiscard]] static esp_err_t spiffs_format(const char* const label = nullptr) noexcept
    {
        return esp_spiffs_format(label);
    }

    [[nodiscard]] static esp_err_t spiffs_check(const char* const label = nullptr) noexcept
    {
        return esp_spiffs_check(label);
    }

    [[nodiscard]] static esp_err_t spiffs_gc(
        const std::size_t bytes,
        const char* const label = nullptr) noexcept
    {
        return esp_spiffs_gc(label, bytes);
    }

    [[nodiscard]] static esp_err_t fat(
        const char* const base = "/fat",
        const char* const label = "storage",
        const int max_files = 4,
        const bool format = false,
        const std::size_t alloc = 0U) noexcept
    {
        if (state.wl != WL_INVALID_HANDLE) {
            return ESP_OK;
        }

        esp_vfs_fat_mount_config_t cfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
        cfg.format_if_mount_failed = format;
        cfg.max_files = max_files;
        cfg.allocation_unit_size = alloc;

        const auto err = esp_vfs_fat_spiflash_mount_rw_wl(base, label, &cfg, &state.wl);
        if (err == ESP_OK) {
            state.base = base;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t fat_ro(
        const char* const base = "/fat",
        const char* const label = "storage",
        const int max_files = 4) noexcept
    {
        esp_vfs_fat_mount_config_t cfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
        cfg.max_files = max_files;
        return esp_vfs_fat_spiflash_mount_ro(base, label, &cfg);
    }

    [[nodiscard]] static esp_err_t fat_off() noexcept
    {
        if (state.wl == WL_INVALID_HANDLE) {
            return ESP_OK;
        }

        const auto err = esp_vfs_fat_spiflash_unmount_rw_wl(state.base, state.wl);
        if (err == ESP_OK) {
            state.wl = WL_INVALID_HANDLE;
            state.base = nullptr;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t fat_ro_off(
        const char* const base = "/fat",
        const char* const label = "storage") noexcept
    {
        return esp_vfs_fat_spiflash_unmount_ro(base, label);
    }

    [[nodiscard]] static esp_err_t fat_info(
        FatInfo& out,
        const char* const base = "/fat") noexcept
    {
        return esp_vfs_fat_info(base, &out.total, &out.free);
    }

    [[nodiscard]] static esp_err_t fat_format(
        const char* const base = "/fat",
        const char* const label = "storage") noexcept
    {
        return esp_vfs_fat_spiflash_format_rw_wl(base, label);
    }

private:
    struct State {
        wl_handle_t wl;
        const char* base;
    };

    constinit static inline State state{WL_INVALID_HANDLE, nullptr};
};

}  // namespace arc
