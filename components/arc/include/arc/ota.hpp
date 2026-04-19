#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_check.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

namespace arc {

struct Ota {
    struct Session {
        const esp_partition_t* slot{};
        esp_ota_handle_t raw{};

        ~Session()
        {
            static_cast<void>(Ota::cancel(*this));
        }

        [[nodiscard]] bool open() const noexcept
        {
            return slot != nullptr;
        }
    };

    [[nodiscard]] static const esp_partition_t* running() noexcept
    {
        return esp_ota_get_running_partition();
    }

    [[nodiscard]] static const esp_partition_t* boot() noexcept
    {
        return esp_ota_get_boot_partition();
    }

    [[nodiscard]] static const esp_partition_t* next() noexcept
    {
        return esp_ota_get_next_update_partition(nullptr);
    }

    [[nodiscard]] static esp_err_t begin(
        Session& session,
        const std::size_t image_size = OTA_WITH_SEQUENTIAL_WRITES,
        const esp_partition_t* slot = nullptr) noexcept
    {
        if (session.open()) {
            return ESP_ERR_INVALID_STATE;
        }

        session.slot = slot != nullptr ? slot : next();
        if (session.slot == nullptr) {
            return ESP_ERR_NOT_FOUND;
        }

        const auto err = esp_ota_begin(session.slot, image_size, &session.raw);
        if (err != ESP_OK) {
            session.slot = nullptr;
        }

        return err;
    }

    [[nodiscard]] static esp_err_t resume(
        Session& session,
        const std::size_t erase_size,
        const std::size_t offset,
        const esp_partition_t* slot = nullptr) noexcept
    {
        if (session.open()) {
            return ESP_ERR_INVALID_STATE;
        }

        session.slot = slot != nullptr ? slot : next();
        if (session.slot == nullptr) {
            return ESP_ERR_NOT_FOUND;
        }

        const auto err = esp_ota_resume(session.slot, erase_size, offset, &session.raw);
        if (err != ESP_OK) {
            session.slot = nullptr;
        }

        return err;
    }

    [[nodiscard]] static esp_err_t set_final(
        Session& session,
        const esp_partition_t* final,
        const bool copy = true) noexcept
    {
        if (!session.open() || final == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_ota_set_final_partition(session.raw, final, copy);
    }

    [[nodiscard]] static esp_err_t write(
        const Session& session,
        const void* data,
        const std::size_t size) noexcept
    {
        if (!session.open()) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_ota_write(session.raw, data, size);
    }

    [[nodiscard]] static esp_err_t write_at(
        const Session& session,
        const void* data,
        const std::size_t size,
        const std::uint32_t offset) noexcept
    {
        if (!session.open()) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_ota_write_with_offset(session.raw, data, size, offset);
    }

    [[nodiscard]] static esp_err_t finish(
        Session& session,
        const bool activate = true) noexcept
    {
        if (!session.open()) {
            return ESP_ERR_INVALID_STATE;
        }

        const auto* const slot = session.slot;
        const auto handle = session.raw;
        session.slot = nullptr;
        session.raw = 0;

        auto err = esp_ota_end(handle);
        if (err != ESP_OK) {
            return err;
        }

        if (activate && slot != nullptr) {
            err = esp_ota_set_boot_partition(slot);
        }

        return err;
    }

    [[nodiscard]] static esp_err_t cancel(Session& session) noexcept
    {
        if (!session.open()) {
            return ESP_OK;
        }

        const auto handle = session.raw;
        session.slot = nullptr;
        session.raw = 0;
        return esp_ota_abort(handle);
    }

    [[nodiscard]] static esp_err_t state(
        const esp_partition_t* part,
        esp_ota_img_states_t& out) noexcept
    {
        if (part == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_ota_get_state_partition(part, &out);
    }

    [[nodiscard]] static bool pending_verify(esp_err_t* err = nullptr) noexcept
    {
        esp_ota_img_states_t out{};
        const auto status = state(running(), out);
        if (err != nullptr) {
            *err = status;
        }
        return status == ESP_OK && out == ESP_OTA_IMG_PENDING_VERIFY;
    }

    [[nodiscard]] static esp_err_t confirm() noexcept
    {
        return esp_ota_mark_app_valid_cancel_rollback();
    }

    [[noreturn]] static void rollback() noexcept
    {
        ESP_ERROR_CHECK(esp_ota_mark_app_invalid_rollback_and_reboot());
        esp_restart();
        for (;;) {
        }
    }
};

}  // namespace arc
