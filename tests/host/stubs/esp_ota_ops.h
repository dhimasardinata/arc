#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_partition.h"

#define OTA_WITH_SEQUENTIAL_WRITES 0xffffffffU

using esp_ota_handle_t = std::uint32_t;

enum esp_ota_img_states_t {
    ESP_OTA_IMG_NEW = 0,
    ESP_OTA_IMG_PENDING_VERIFY = 1,
    ESP_OTA_IMG_VALID = 2,
    ESP_OTA_IMG_INVALID = 3,
    ESP_OTA_IMG_ABORTED = 4,
};

inline esp_partition_t esp_ota_stub_running{.subtype = 1};
inline esp_partition_t esp_ota_stub_boot{.subtype = 1};
inline esp_partition_t esp_ota_stub_next{.subtype = 2};
inline esp_ota_img_states_t esp_ota_stub_state{ESP_OTA_IMG_VALID};
inline esp_err_t esp_ota_stub_begin_result{ESP_OK};
inline esp_err_t esp_ota_stub_resume_result{ESP_OK};
inline esp_err_t esp_ota_stub_write_result{ESP_OK};
inline esp_err_t esp_ota_stub_write_at_result{ESP_OK};
inline esp_err_t esp_ota_stub_end_result{ESP_OK};
inline esp_err_t esp_ota_stub_boot_result{ESP_OK};
inline esp_err_t esp_ota_stub_abort_result{ESP_OK};
inline esp_err_t esp_ota_stub_state_result{ESP_OK};
inline std::size_t esp_ota_stub_written{};
inline std::size_t esp_ota_stub_written_at{};
inline std::uint32_t esp_ota_stub_last_offset{};
inline std::uint32_t esp_ota_stub_aborted{};
inline std::uint32_t esp_ota_stub_boot_sets{};
inline esp_ota_handle_t esp_ota_stub_next_handle{1U};

inline void esp_ota_stub_reset()
{
    esp_ota_stub_state = ESP_OTA_IMG_VALID;
    esp_ota_stub_begin_result = ESP_OK;
    esp_ota_stub_resume_result = ESP_OK;
    esp_ota_stub_write_result = ESP_OK;
    esp_ota_stub_write_at_result = ESP_OK;
    esp_ota_stub_end_result = ESP_OK;
    esp_ota_stub_boot_result = ESP_OK;
    esp_ota_stub_abort_result = ESP_OK;
    esp_ota_stub_state_result = ESP_OK;
    esp_ota_stub_written = 0U;
    esp_ota_stub_written_at = 0U;
    esp_ota_stub_last_offset = 0U;
    esp_ota_stub_aborted = 0U;
    esp_ota_stub_boot_sets = 0U;
    esp_ota_stub_next_handle = 1U;
}

inline const esp_partition_t* esp_ota_get_running_partition()
{
    return &esp_ota_stub_running;
}

inline const esp_partition_t* esp_ota_get_boot_partition()
{
    return &esp_ota_stub_boot;
}

inline const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*)
{
    return &esp_ota_stub_next;
}

inline esp_err_t esp_ota_begin(
    const esp_partition_t* const partition,
    const std::size_t,
    esp_ota_handle_t* const handle)
{
    if (partition == nullptr || handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_begin_result == ESP_OK) {
        *handle = esp_ota_stub_next_handle++;
    }
    return esp_ota_stub_begin_result;
}

inline esp_err_t esp_ota_resume(
    const esp_partition_t* const partition,
    const std::size_t,
    const std::size_t,
    esp_ota_handle_t* const handle)
{
    if (partition == nullptr || handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_resume_result == ESP_OK) {
        *handle = esp_ota_stub_next_handle++;
    }
    return esp_ota_stub_resume_result;
}

inline esp_err_t esp_ota_set_final_partition(
    const esp_ota_handle_t handle,
    const esp_partition_t* const final,
    const bool)
{
    return handle != 0U && final != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_ota_write(
    const esp_ota_handle_t handle,
    const void* const data,
    const std::size_t size)
{
    if (handle == 0U || (data == nullptr && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_write_result == ESP_OK) {
        esp_ota_stub_written += size;
    }
    return esp_ota_stub_write_result;
}

inline esp_err_t esp_ota_write_with_offset(
    const esp_ota_handle_t handle,
    const void* const data,
    const std::size_t size,
    const std::uint32_t offset)
{
    if (handle == 0U || (data == nullptr && size != 0U)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_write_at_result == ESP_OK) {
        esp_ota_stub_written_at += size;
        esp_ota_stub_last_offset = offset;
    }
    return esp_ota_stub_write_at_result;
}

inline esp_err_t esp_ota_end(const esp_ota_handle_t handle)
{
    return handle != 0U ? esp_ota_stub_end_result : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_ota_set_boot_partition(const esp_partition_t* const partition)
{
    if (partition == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_boot_result == ESP_OK) {
        ++esp_ota_stub_boot_sets;
    }
    return esp_ota_stub_boot_result;
}

inline esp_err_t esp_ota_abort(const esp_ota_handle_t handle)
{
    if (handle == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_abort_result == ESP_OK) {
        ++esp_ota_stub_aborted;
    }
    return esp_ota_stub_abort_result;
}

inline esp_err_t esp_ota_get_state_partition(
    const esp_partition_t* const partition,
    esp_ota_img_states_t* const out)
{
    if (partition == nullptr || out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (esp_ota_stub_state_result == ESP_OK) {
        *out = esp_ota_stub_state;
    }
    return esp_ota_stub_state_result;
}

inline esp_err_t esp_ota_mark_app_valid_cancel_rollback()
{
    return ESP_OK;
}

inline esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot()
{
    return ESP_OK;
}
