#pragma once

#include <cstdint>

#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

namespace arc {

struct Space {
    static void heap(const char* tag = "arc-space", const char* title = nullptr) noexcept
    {
        head(tag, title);
        row(tag, "8bit", MALLOC_CAP_8BIT);
        row(tag, "internal", MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        row(tag, "dma", MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        row(tag, "simd", MALLOC_CAP_SIMD | MALLOC_CAP_INTERNAL);
#if CONFIG_HEAP_HAS_EXEC_HEAP
        row(tag, "exec", MALLOC_CAP_EXEC);
#endif
        row(tag, "iram8", MALLOC_CAP_IRAM_8BIT);
        row(tag, "rtcram", MALLOC_CAP_RTCRAM | MALLOC_CAP_8BIT);
        row(tag, "psram", MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }

    static void flash(const char* tag = "arc-space", const char* title = nullptr) noexcept
    {
        head(tag, title);

        std::uint32_t chip = 0;
        const auto err = esp_flash_get_size(nullptr, &chip);
        if (err != ESP_OK) {
            ESP_LOGW(tag, "flash size unavailable err=0x%x", static_cast<unsigned>(err));
            return;
        }

        ESP_LOGI(tag, "flash chip=%uB (%u KiB, %u MiB)", chip, kib(chip), mib(chip));

        if (const auto* const running = esp_ota_get_running_partition(); running != nullptr) {
            const auto slot = pct(static_cast<std::uint32_t>(running->size), chip);
            ESP_LOGI(tag,
                     "running label=%s addr=0x%08x size=%uB (%u KiB, %u.%02u%% chip)",
                     running->label,
                     running->address,
                     running->size,
                     kib(running->size),
                     slot.whole,
                     slot.frac);

            esp_ota_img_states_t state{};
            if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
                ESP_LOGI(tag, "running state=%s", state_name(state));
            }

            const auto app = app_area();
            const auto image = image_size(*running);
            if (image != 0U) {
                const auto in_slot = pct(image, static_cast<std::uint32_t>(running->size));
                const auto in_apps = pct(image, app.total);
                const auto slot_free =
                    running->size > image ? static_cast<std::uint32_t>(running->size) - image : 0U;
                const auto free_slot = pct(slot_free, static_cast<std::uint32_t>(running->size));

                ESP_LOGI(tag,
                         "image=%uB (%u KiB, %u.%02u%% slot, %u.%02u%% ota apps) slot_free=%uB (%u KiB, %u.%02u%% slot)",
                         image,
                         kib(image),
                         in_slot.whole,
                         in_slot.frac,
                         in_apps.whole,
                         in_apps.frac,
                         slot_free,
                         kib(slot_free),
                         free_slot.whole,
                         free_slot.frac);
            }

            if (app.total != 0U) {
                const auto apps = pct(app.total, chip);
                ESP_LOGI(tag,
                         "app area slots=%u total=%uB (%u KiB, %u.%02u%% chip)",
                         app.count,
                         app.total,
                         kib(app.total),
                         apps.whole,
                         apps.frac);
            }
        }

        if (const auto* const boot = esp_ota_get_boot_partition(); boot != nullptr) {
            ESP_LOGI(tag,
                     "boot label=%s addr=0x%08x size=%uB (%u KiB)",
                     boot->label,
                     boot->address,
                     boot->size,
                     kib(boot->size));
        }

        if (const auto* const next = esp_ota_get_next_update_partition(nullptr); next != nullptr) {
            ESP_LOGI(tag,
                     "next update label=%s addr=0x%08x size=%uB (%u KiB)",
                     next->label,
                     next->address,
                     next->size,
                     kib(next->size));
        }
    }

    static void parts(const char* tag = "arc-space", const char* title = nullptr) noexcept
    {
        head(tag, title);

        std::uint32_t chip = 0;
        static_cast<void>(esp_flash_get_size(nullptr, &chip));

        std::uint64_t total = 0;
        std::uint32_t top = 0;
        std::uint32_t count = 0;

        for (auto it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
             it != nullptr;
             it = esp_partition_next(it)) {
            const auto* const part = esp_partition_get(it);
            if (part != nullptr) {
                const auto end = static_cast<std::uint32_t>(part->address + part->size);
                total += static_cast<std::uint32_t>(part->size);
                top = end > top ? end : top;
                ++count;

                ESP_LOGI(tag,
                         "part %-12s kind=%s/%s addr=0x%08x size=%uB (%u KiB)",
                         part->label,
                         type_name(part->type),
                         subtype_name(*part),
                         part->address,
                         part->size,
                         kib(part->size));
            }
        }

        if (chip == 0U) {
            ESP_LOGI(
                tag, "partitions count=%u total=%lluB top=0x%08x", count, static_cast<unsigned long long>(total), top);
            return;
        }

        const auto used = pct(static_cast<std::uint32_t>(total), chip);
        const auto filled = pct(top, chip);
        const auto free = chip > top ? chip - top : 0U;
        ESP_LOGI(tag,
                 "partitions count=%u total=%lluB (%u.%02u%% chip) top=0x%08x (%u.%02u%% chip) tail_free=%uB (%u KiB)",
                 count,
                 static_cast<unsigned long long>(total),
                 used.whole,
                 used.frac,
                 top,
                 filled.whole,
                 filled.frac,
                 free,
                 kib(free));
    }

    static void all(const char* tag = "arc-space", const char* title = nullptr) noexcept
    {
        flash(tag, title);
        parts(tag);
        heap(tag);
    }

private:
    struct Percent {
        std::uint32_t whole;
        std::uint32_t frac;
    };

    struct AppArea {
        std::uint32_t total;
        std::uint32_t count;
    };

    [[nodiscard]] static constexpr std::uint32_t kib(const std::uint32_t bytes) noexcept
    {
        return bytes / 1024U;
    }

    [[nodiscard]] static constexpr std::uint32_t mib(const std::uint32_t bytes) noexcept
    {
        return bytes / (1024U * 1024U);
    }

    [[nodiscard]] static Percent pct(const std::uint32_t value, const std::uint32_t total) noexcept
    {
        if (total == 0U) {
            return {};
        }

        const auto scaled = (static_cast<std::uint64_t>(value) * 10000ULL) / total;
        return Percent{
            .whole = static_cast<std::uint32_t>(scaled / 100ULL),
            .frac = static_cast<std::uint32_t>(scaled % 100ULL),
        };
    }

    [[nodiscard]] static std::uint32_t image_size(const esp_partition_t& part) noexcept
    {
        const esp_partition_pos_t pos{
            .offset = part.address,
            .size = part.size,
        };
        esp_image_metadata_t meta{};
        const auto err = esp_image_get_metadata(&pos, &meta);
        return err == ESP_OK ? meta.image_len : 0U;
    }

    [[nodiscard]] static AppArea app_area() noexcept
    {
        AppArea area{};
        for (auto it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
             it != nullptr;
             it = esp_partition_next(it)) {
            const auto* const part = esp_partition_get(it);
            if (part == nullptr) {
                continue;
            }

            area.total += static_cast<std::uint32_t>(part->size);
            ++area.count;
        }
        return area;
    }

    static void head(const char* tag, const char* title) noexcept
    {
        if (title != nullptr && title[0] != '\0') {
            ESP_LOGI(tag, "%s", title);
        }
    }

    static void row(const char* tag, const char* label, const std::uint32_t caps) noexcept
    {
        const auto total = static_cast<std::uint32_t>(heap_caps_get_total_size(caps));
        if (total == 0U) {
            return;
        }

        const auto free = static_cast<std::uint32_t>(heap_caps_get_free_size(caps));
        const auto min = static_cast<std::uint32_t>(heap_caps_get_minimum_free_size(caps));
        const auto largest = static_cast<std::uint32_t>(heap_caps_get_largest_free_block(caps));
        const auto used = total - free;
        const auto fill = pct(used, total);

        ESP_LOGI(tag,
                 "%-8s total=%uB free=%uB used=%uB (%u.%02u%%) min=%uB largest=%uB",
                 label,
                 total,
                 free,
                 used,
                 fill.whole,
                 fill.frac,
                 min,
                 largest);
    }

    [[nodiscard]] static constexpr const char* type_name(const esp_partition_type_t type) noexcept
    {
        switch (type) {
            case ESP_PARTITION_TYPE_APP:
                return "app";
            case ESP_PARTITION_TYPE_DATA:
                return "data";
            default:
                return "raw";
        }
    }

    [[nodiscard]] static constexpr const char* subtype_name(const esp_partition_t& part) noexcept
    {
        if (part.type == ESP_PARTITION_TYPE_APP) {
            if (part.subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
                return "factory";
            }
            if (part.subtype == ESP_PARTITION_SUBTYPE_APP_TEST) {
                return "test";
            }
            if (part.subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
                part.subtype <= ESP_PARTITION_SUBTYPE_APP_OTA_MAX) {
                return "ota";
            }
            return "app";
        }

        if (part.type == ESP_PARTITION_TYPE_DATA) {
            switch (part.subtype) {
                case ESP_PARTITION_SUBTYPE_DATA_NVS:
                    return "nvs";
                case ESP_PARTITION_SUBTYPE_DATA_OTA:
                    return "ota";
                case ESP_PARTITION_SUBTYPE_DATA_PHY:
                    return "phy";
                case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
                    return "spiffs";
                case ESP_PARTITION_SUBTYPE_DATA_FAT:
                    return "fat";
                default:
                    return "data";
            }
        }

        return "raw";
    }

    [[nodiscard]] static constexpr const char* state_name(const esp_ota_img_states_t state) noexcept
    {
        switch (state) {
            case ESP_OTA_IMG_NEW:
                return "new";
            case ESP_OTA_IMG_PENDING_VERIFY:
                return "pending_verify";
            case ESP_OTA_IMG_VALID:
                return "valid";
            case ESP_OTA_IMG_INVALID:
                return "invalid";
            case ESP_OTA_IMG_ABORTED:
                return "aborted";
            case ESP_OTA_IMG_UNDEFINED:
            default:
                return "undefined";
        }
    }
};

}  // namespace arc
