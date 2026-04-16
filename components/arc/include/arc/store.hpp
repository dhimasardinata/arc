#pragma once

#include <cstddef>
#include <type_traits>

#include "nvs.h"
#include "nvs_flash.h"

namespace arc {

struct Store {
    static esp_err_t boot() noexcept
    {
        auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            err = nvs_flash_erase();
            if (err != ESP_OK) {
                return err;
            }
            err = nvs_flash_init();
        }
        return err;
    }

    template <typename T>
    [[nodiscard]] static esp_err_t save(const char* ns, const char* key, const T& value) noexcept
    {
        static_assert(blob<T>(), "Store values must be trivially copyable");

        Handle handle{};
        auto err = handle.open(ns, NVS_READWRITE);
        if (err != ESP_OK) {
            return err;
        }

        err = nvs_set_blob(handle.raw, key, &value, sizeof(T));
        if (err != ESP_OK) {
            return err;
        }

        return nvs_commit(handle.raw);
    }

    template <typename T>
    [[nodiscard]] static esp_err_t load(const char* ns, const char* key, T& value) noexcept
    {
        static_assert(blob<T>(), "Store values must be trivially copyable");

        Handle handle{};
        auto err = handle.open(ns, NVS_READONLY);
        if (err != ESP_OK) {
            return err;
        }

        std::size_t size = sizeof(T);
        err = nvs_get_blob(handle.raw, key, &value, &size);
        if (err != ESP_OK) {
            return err;
        }

        return size == sizeof(T) ? ESP_OK : ESP_ERR_NVS_INVALID_LENGTH;
    }

    template <typename T>
    [[nodiscard]] static T load_or(
        const char* ns,
        const char* key,
        const T& fallback,
        esp_err_t* err = nullptr) noexcept
    {
        static_assert(blob<T>(), "Store values must be trivially copyable");

        T value = fallback;
        const auto status = load(ns, key, value);
        if (err != nullptr) {
            *err = status;
        }

        return status == ESP_OK ? value : fallback;
    }

    [[nodiscard]] static esp_err_t erase(const char* ns, const char* key) noexcept
    {
        Handle handle{};
        auto err = handle.open(ns, NVS_READWRITE);
        if (err != ESP_OK) {
            return err;
        }

        err = nvs_erase_key(handle.raw, key);
        if (err != ESP_OK) {
            return err;
        }

        return nvs_commit(handle.raw);
    }

private:
    template <typename T>
    [[nodiscard]] static consteval bool blob() noexcept
    {
        return std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>;
    }

    struct Handle {
        nvs_handle_t raw{};

        ~Handle()
        {
            if (raw != 0) {
                nvs_close(raw);
            }
        }

        [[nodiscard]] esp_err_t open(const char* ns, const nvs_open_mode_t mode) noexcept
        {
            return nvs_open(ns, mode, &raw);
        }
    };
};

}  // namespace arc
