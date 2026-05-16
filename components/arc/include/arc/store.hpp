#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "nvs.h"
#include "nvs_flash.h"

#include "arc/init.hpp"
#include "arc/result.hpp"

namespace arc {

template <std::size_t N>
struct StoreText {
    static_assert(N > 0U, "StoreText needs room for a NUL terminator");

    std::array<char, N> bytes{};
    std::size_t chars{};

    [[nodiscard]] static consteval std::size_t cap() noexcept
    {
        return N - 1U;
    }

    [[nodiscard]] constexpr std::span<char, N> span() noexcept
    {
        return std::span<char, N>{bytes};
    }

    [[nodiscard]] constexpr std::span<const char, N> span() const noexcept
    {
        return std::span<const char, N>{bytes};
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return std::string_view{bytes.data(), chars};
    }

    [[nodiscard]] constexpr const char* c_str() const noexcept
    {
        return bytes.data();
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return chars;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return chars == 0U;
    }

    [[nodiscard]] constexpr esp_err_t set(const std::string_view value) noexcept
    {
        if (value.size() > cap()) {
            return ESP_ERR_NVS_INVALID_LENGTH;
        }

        for (std::size_t i = 0U; i < value.size(); ++i) {
            bytes[i] = value[i];
        }

        bytes[value.size()] = '\0';
        chars = value.size();
        return ESP_OK;
    }
};

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

    template <std::size_t N>
    [[nodiscard]] static esp_err_t save_text(
        const char* ns,
        const char* key,
        const StoreText<N>& value) noexcept
    {
        return save_string(ns, key, value.c_str());
    }

    template <std::size_t N>
    [[nodiscard]] static esp_err_t save_text(
        const char* ns,
        const char* key,
        const std::string_view value) noexcept
    {
        StoreText<N> text{};
        const auto err = text.set(value);
        if (err != ESP_OK) {
            return err;
        }

        return save_text(ns, key, text);
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

    template <std::size_t Extent>
    [[nodiscard]] static esp_err_t load_string(
        const char* ns,
        const char* key,
        const std::span<char, Extent> out,
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

    template <std::size_t N>
    [[nodiscard]] static Result<StoreText<N>> load_text(
        const char* ns,
        const char* key,
        const std::string_view fallback = {},
        esp_err_t* err = nullptr) noexcept
    {
        StoreText<N> out{};
        auto status = out.set(fallback);
        if (status != ESP_OK) {
            report(err, status);
            return fail(status);
        }

        std::size_t bytes{};
        status = string_size(ns, key, bytes);
        if (status == ESP_ERR_NVS_NOT_FOUND) {
            report(err, status);
            return ok(out);
        }
        if (status != ESP_OK) {
            report(err, status);
            return fail(status);
        }
        if (bytes > out.bytes.size()) {
            report(err, ESP_ERR_NVS_INVALID_LENGTH);
            return fail(ESP_ERR_NVS_INVALID_LENGTH);
        }

        status = load_string(ns, key, out.span(), &out.chars);
        report(err, status);
        if (status != ESP_OK) {
            return fail(status);
        }

        return ok(out);
    }

private:
    constinit static inline std::uint32_t init{};

    static void report(esp_err_t* err, const esp_err_t status) noexcept
    {
        if (err != nullptr) {
            *err = status;
        }
    }

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
