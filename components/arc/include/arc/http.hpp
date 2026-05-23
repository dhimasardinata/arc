#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_err.h"
#include "esp_http_client.h"

#include "arc/result.hpp"
#include "arc/uri.hpp"

namespace arc::net {

template <typename T>
concept HttpBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct Http {
    constexpr Http() noexcept = default;

    explicit Http(esp_http_client_handle_t const handle, const bool secure_only = false) noexcept
        : handle_(handle)
        , secure_only_(secure_only)
    {
    }

    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;

    Http(Http&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
        , secure_only_(std::exchange(other.secure_only_, false))
    {
    }

    Http& operator=(Http&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(close());
            cleanup();
            handle_ = std::exchange(other.handle_, nullptr);
            secure_only_ = std::exchange(other.secure_only_, false);
        }
        return *this;
    }

    ~Http()
    {
        static_cast<void>(close());
        cleanup();
    }

    [[nodiscard]] static Result<Http> make(const esp_http_client_config_t& config) noexcept
    {
        return init(config, false);
    }

    [[nodiscard]] static Result<Http> https(const esp_http_client_config_t& config) noexcept
    {
        if (!secure_url(config.url)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return init(config, true);
    }

    [[nodiscard]] static Result<Http> https(
        const char* const url,
        const esp_http_client_config_t& base = {}) noexcept
    {
        if (!secure_url(url)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto config = base;
        config.url = url;
        return init(config, true);
    }

    [[nodiscard]] esp_err_t url(const char* const value) noexcept
    {
        if (handle_ == nullptr || value == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        if (secure_only_ && !secure_url(value)) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_http_client_set_url(handle_, value);
    }

    [[nodiscard]] esp_err_t method(const esp_http_client_method_t value) noexcept
    {
        if (handle_ == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_http_client_set_method(handle_, value);
    }

    [[nodiscard]] esp_err_t header(const char* const key, const char* const value) noexcept
    {
        if (handle_ == nullptr || key == nullptr || value == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_http_client_set_header(handle_, key, value);
    }

    [[nodiscard]] esp_err_t body(const void* const data, const std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || (data == nullptr && bytes != 0U) ||
            bytes > static_cast<std::size_t>(INT_MAX)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (bytes == 0U) {
            return ESP_OK;
        }
        return esp_http_client_set_post_field(
            handle_,
            static_cast<const char*>(data),
            static_cast<int>(bytes));
    }

    template <typename T, std::size_t Extent>
        requires HttpBytes<T>
    [[nodiscard]] esp_err_t body(const std::span<T, Extent> data) noexcept
    {
        return body(data.data(), data.size_bytes());
    }

    [[nodiscard]] esp_err_t perform() noexcept
    {
        if (handle_ == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_http_client_perform(handle_);
    }

    [[nodiscard]] esp_err_t open(const std::size_t write_bytes = 0U) noexcept
    {
        if (handle_ == nullptr || write_bytes > static_cast<std::size_t>(INT_MAX)) {
            return ESP_ERR_INVALID_ARG;
        }
        return esp_http_client_open(handle_, static_cast<int>(write_bytes));
    }

    [[nodiscard]] Result<std::int64_t> fetch() noexcept
    {
        if (handle_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto len = esp_http_client_fetch_headers(handle_);
        if (len < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::int64_t>(len);
    }

    [[nodiscard]] Result<std::size_t> write(const void* const data, const std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || (data == nullptr && bytes != 0U) ||
            bytes > static_cast<std::size_t>(INT_MAX)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (bytes == 0U) {
            return 0U;
        }

        const auto sent = esp_http_client_write(
            handle_,
            static_cast<const char*>(data),
            static_cast<int>(bytes));
        if (sent < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::size_t>(sent);
    }

    template <typename T, std::size_t Extent>
        requires HttpBytes<T>
    [[nodiscard]] Result<std::size_t> write(const std::span<T, Extent> data) noexcept
    {
        return write(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<std::size_t> read(void* const data, const std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || (data == nullptr && bytes != 0U) ||
            bytes > static_cast<std::size_t>(INT_MAX)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (bytes == 0U) {
            return 0U;
        }

        const auto got = esp_http_client_read(
            handle_,
            static_cast<char*>(data),
            static_cast<int>(bytes));
        if (got < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::size_t>(got);
    }

    template <typename T, std::size_t Extent>
        requires(HttpBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> read(const std::span<T, Extent> data) noexcept
    {
        return read(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<int> status() noexcept
    {
        if (handle_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return esp_http_client_get_status_code(handle_);
    }

    [[nodiscard]] Result<std::int64_t> length() noexcept
    {
        if (handle_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return esp_http_client_get_content_length(handle_);
    }

    [[nodiscard]] esp_err_t close() noexcept
    {
        if (handle_ == nullptr) {
            return ESP_OK;
        }
        return esp_http_client_close(handle_);
    }

    [[nodiscard]] esp_http_client_handle_t native() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

private:
    [[nodiscard]] static Result<Http> init(
        const esp_http_client_config_t& config,
        const bool secure_only) noexcept
    {
        auto* const handle = esp_http_client_init(&config);
        if (handle == nullptr) {
            return fail(ESP_ERR_NO_MEM);
        }
        return Http{handle, secure_only};
    }

    [[nodiscard]] static bool secure_url(const char* const url) noexcept
    {
        const auto uri = Uri::parse(url);
        return uri.has_value() && Uri::scheme_is(*uri, "https") && Uri::endpoint(*uri, 443U).has_value();
    }

    void cleanup() noexcept
    {
        if (handle_ != nullptr) {
            static_cast<void>(esp_http_client_cleanup(handle_));
            handle_ = nullptr;
        }
    }

    esp_http_client_handle_t handle_{nullptr};
    bool secure_only_{false};
};

}  // namespace arc::net
