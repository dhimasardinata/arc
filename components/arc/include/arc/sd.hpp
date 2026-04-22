#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Clk = 14,
          int Cmd = 15,
          int D0 = 2,
          int D1 = 4,
          int D2 = 12,
          int D3 = 13,
          int Width = 4,
          int Cd = SDMMC_SLOT_NO_CD,
          int Wp = SDMMC_SLOT_NO_WP,
          std::uint32_t Hz = SDMMC_FREQ_DEFAULT,
          bool Pullup = false,
          int Slot = SDMMC_HOST_SLOT_1,
          int MaxFiles = 5,
          bool Format = false,
          std::size_t Alloc = 0U>
struct Sd {
    static_assert(SOC_SDMMC_HOST_SUPPORTED, "arc::Sd requires ESP32-S3 SDMMC host");
    static_assert(Width == 1 || Width == 4, "ESP32-S3 SD slot width must be 1 or 4");
    static_assert(Slot == SDMMC_HOST_SLOT_0 || Slot == SDMMC_HOST_SLOT_1, "invalid SDMMC slot");
    static_assert(Clk >= 0 && Clk < SOC_GPIO_PIN_COUNT, "invalid SDMMC CLK pin");
    static_assert(Cmd >= 0 && Cmd < SOC_GPIO_PIN_COUNT, "invalid SDMMC CMD pin");
    static_assert(D0 >= 0 && D0 < SOC_GPIO_PIN_COUNT, "invalid SDMMC D0 pin");
    static_assert(
        Width == 1 ||
            ((D1 >= 0 && D1 < SOC_GPIO_PIN_COUNT) &&
             (D2 >= 0 && D2 < SOC_GPIO_PIN_COUNT) &&
             (D3 >= 0 && D3 < SOC_GPIO_PIN_COUNT)),
        "invalid 4-bit SDMMC data pins");
    static_assert(Cd == SDMMC_SLOT_NO_CD || (Cd >= 0 && Cd < SOC_GPIO_PIN_COUNT), "invalid SDMMC card-detect pin");
    static_assert(Wp == SDMMC_SLOT_NO_WP || (Wp >= 0 && Wp < SOC_GPIO_PIN_COUNT), "invalid SDMMC write-protect pin");
    static_assert(Hz > 0U, "SDMMC frequency must be non-zero");
    static_assert(MaxFiles > 0, "SD FAT max file count must be non-zero");

    [[nodiscard]] static esp_err_t mount(const char* const base = "/sd") noexcept
    {
        if (state.card != nullptr) {
            return ESP_OK;
        }

        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        host.slot = Slot;
        host.max_freq_khz = Hz;

        sdmmc_slot_config_t slot{};
        slot.clk = static_cast<gpio_num_t>(Clk);
        slot.cmd = static_cast<gpio_num_t>(Cmd);
        slot.d0 = static_cast<gpio_num_t>(D0);
        slot.d1 = data_pin<D1>();
        slot.d2 = data_pin<D2>();
        slot.d3 = data_pin<D3>();
        slot.d4 = GPIO_NUM_NC;
        slot.d5 = GPIO_NUM_NC;
        slot.d6 = GPIO_NUM_NC;
        slot.d7 = GPIO_NUM_NC;
        slot.cd = static_cast<gpio_num_t>(Cd);
        slot.wp = static_cast<gpio_num_t>(Wp);
        slot.width = static_cast<std::uint8_t>(Width);
        slot.flags = Pullup ? SDMMC_SLOT_FLAG_INTERNAL_PULLUP : 0U;

        esp_vfs_fat_mount_config_t cfg = VFS_FAT_MOUNT_DEFAULT_CONFIG();
        cfg.format_if_mount_failed = Format;
        cfg.max_files = MaxFiles;
        cfg.allocation_unit_size = Alloc;

        const auto err = esp_vfs_fat_sdmmc_mount(base, &host, &slot, &cfg, &state.card);
        if (err == ESP_OK) {
            state.base = base;
        }
        return err;
    }

    static void boot(const char* const base = "/sd")
    {
        ESP_ERROR_CHECK(mount(base));
    }

    [[nodiscard]] static esp_err_t unmount() noexcept
    {
        if (state.card == nullptr) {
            return ESP_OK;
        }

        const auto err = esp_vfs_fat_sdcard_unmount(state.base, state.card);
        if (err == ESP_OK) {
            state.card = nullptr;
            state.base = nullptr;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t status() noexcept
    {
        if (state.card == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return sdmmc_get_status(state.card);
    }

    [[nodiscard]] static esp_err_t read(
        void* const dst,
        const std::size_t start,
        const std::size_t sectors) noexcept
    {
        if (state.card == nullptr || dst == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return sdmmc_read_sectors(state.card, dst, start, sectors);
    }

    [[nodiscard]] static esp_err_t write(
        const void* const src,
        const std::size_t start,
        const std::size_t sectors) noexcept
    {
        if (state.card == nullptr || src == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return sdmmc_write_sectors(state.card, src, start, sectors);
    }

    [[nodiscard]] static esp_err_t format() noexcept
    {
        if (state.card == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_vfs_fat_sdcard_format(state.base, state.card);
    }

    [[nodiscard]] static sdmmc_card_t* card() noexcept
    {
        return state.card;
    }

    [[nodiscard]] static std::size_t sector() noexcept
    {
        return state.card == nullptr ? 0U : static_cast<std::size_t>(state.card->csd.sector_size);
    }

    [[nodiscard]] static std::uint64_t bytes() noexcept
    {
        if (state.card == nullptr) {
            return 0U;
        }
        return static_cast<std::uint64_t>(state.card->csd.capacity) *
               static_cast<std::uint64_t>(state.card->csd.sector_size);
    }

    [[nodiscard]] static constexpr int width() noexcept
    {
        return Width;
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

private:
    template <int Pin>
    [[nodiscard]] static constexpr gpio_num_t data_pin() noexcept
    {
        if constexpr (Width == 1) {
            return GPIO_NUM_NC;
        } else {
            return static_cast<gpio_num_t>(Pin);
        }
    }

    struct State {
        sdmmc_card_t* card{};
        const char* base{};
    };

    constinit static inline State state{};
};

}  // namespace arc
