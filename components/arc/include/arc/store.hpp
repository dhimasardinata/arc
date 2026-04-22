#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "nvs.h"
#include "nvs_flash.h"

#include "arc/init.hpp"

namespace arc {

struct Store {
    static esp_err_t boot() noexcept
    {
        if (!Init::begin(init)) {
            return ESP_OK;
        }

        auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            err = nvs_flash_erase();
            if (err != ESP_OK) {
                Init::fail(init);
                return err;
            }
            err = nvs_flash_init();
        }

        if (err == ESP_OK) {
            Init::pass(init);
        } else {
            Init::fail(init);
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

    [[nodiscard]] static esp_err_t save_string(
        const char* ns,
        const char* key,
        const char* value) noexcept
    {
        if (value == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        Handle handle{};
        auto err = handle.open(ns, NVS_READWRITE);
        if (err != ESP_OK) {
            return err;
        }

        err = nvs_set_str(handle.raw, key, value);
        if (err != ESP_OK) {
            return err;
        }

        return nvs_commit(handle.raw);
    }

    [[nodiscard]] static esp_err_t string_size(
        const char* ns,
        const char* key,
        std::size_t& bytes) noexcept
    {
        bytes = 0U;

        Handle handle{};
        const auto err = handle.open(ns, NVS_READONLY);
        if (err != ESP_OK) {
            return err;
        }

        return nvs_get_str(handle.raw, key, nullptr, &bytes);
    }

    [[nodiscard]] static esp_err_t load_string(
        const char* ns,
        const char* key,
        const std::span<char> out,
        std::size_t* chars = nullptr) noexcept
    {
        if (out.empty()) {
            return ESP_ERR_INVALID_ARG;
        }

        Handle handle{};
        auto err = handle.open(ns, NVS_READONLY);
        if (err != ESP_OK) {
            return err;
        }

        std::size_t bytes = out.size_bytes();
        err = nvs_get_str(handle.raw, key, out.data(), &bytes);
        if (err != ESP_OK) {
            return err;
        }

        if (chars != nullptr) {
            *chars = bytes == 0U ? 0U : bytes - 1U;
        }

        return ESP_OK;
    }

private:
    constinit static inline std::uint32_t init{};

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
