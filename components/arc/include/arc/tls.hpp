#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#include "esp_tls.h"
#include "esp_err.h"

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept TlsBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct Tls {
    constexpr Tls() noexcept = default;

    explicit Tls(esp_tls_t* const handle) noexcept
        : handle_(handle)
    {
    }

    Tls(const Tls&) = delete;
    Tls& operator=(const Tls&) = delete;

    Tls(Tls&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    Tls& operator=(Tls&& other) noexcept
    {
        if (this != &other) {
            close();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~Tls()
    {
        close();
    }

    [[nodiscard]] static Result<Tls> dial(
        const char* const host,
        const std::uint16_t port,
        const esp_tls_cfg_t& cfg) noexcept
    {
        if (host == nullptr || port == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto host_len = std::strlen(host);
        if (host_len == 0U || host_len > static_cast<std::size_t>(INT_MAX)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto* const handle = esp_tls_init();
        if (handle == nullptr) {
            return fail(ESP_ERR_NO_MEM);
        }

        const auto connected = esp_tls_conn_new_sync(
            host,
            static_cast<int>(host_len),
            static_cast<int>(port),
            &cfg,
            handle);
        if (connected != 1) {
            static_cast<void>(esp_tls_conn_destroy(handle));
            return fail(connected == 0 ? ESP_ERR_TIMEOUT : ESP_FAIL);
        }

        return Tls{handle};
    }

    [[nodiscard]] Result<std::size_t> send(const void* const data, const std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || data == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto sent = esp_tls_conn_write(handle_, data, bytes);
        if (sent < 0) {
            return fail(io_error(sent));
        }
        return static_cast<std::size_t>(sent);
    }

    template <typename T, std::size_t Extent>
        requires TlsBytes<T>
    [[nodiscard]] Result<std::size_t> send(const std::span<T, Extent> data) noexcept
    {
        return send(data.data(), data.size_bytes());
    }

    [[nodiscard]] Status send_all(const void* data, std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || data == nullptr) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }

        auto* cursor = static_cast<const std::uint8_t*>(data);
        while (bytes != 0U) {
            const auto sent = send(cursor, bytes);
            if (!sent) {
                return Status{fail(sent.error())};
            }
            if (*sent == 0U) {
                return Status{fail(ESP_FAIL)};
            }
            cursor += *sent;
            bytes -= *sent;
        }
        return ok();
    }

    template <typename T, std::size_t Extent>
        requires TlsBytes<T>
    [[nodiscard]] Status send_all(const std::span<T, Extent> data) noexcept
    {
        return send_all(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<std::size_t> recv(void* const data, const std::size_t bytes) noexcept
    {
        if (handle_ == nullptr || data == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto got = esp_tls_conn_read(handle_, data, bytes);
        if (got < 0) {
            return fail(io_error(got));
        }
        return static_cast<std::size_t>(got);
    }

    template <typename T, std::size_t Extent>
        requires(TlsBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> recv(const std::span<T, Extent> data) noexcept
    {
        return recv(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<std::size_t> avail() noexcept
    {
        if (handle_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto bytes = esp_tls_get_bytes_avail(handle_);
        if (bytes < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::size_t>(bytes);
    }

    void close() noexcept
    {
        if (handle_ != nullptr) {
            static_cast<void>(esp_tls_conn_destroy(std::exchange(handle_, nullptr)));
        }
    }

    [[nodiscard]] esp_tls_t* native() const noexcept
    {
        return handle_;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

private:
    [[nodiscard]] static esp_err_t io_error(const ssize_t err) noexcept
    {
        switch (err) {
            case ESP_TLS_ERR_SSL_WANT_READ:
            case ESP_TLS_ERR_SSL_WANT_WRITE:
                return ESP_ERR_TIMEOUT;
            default:
                return ESP_FAIL;
        }
    }

    esp_tls_t* handle_{};
};

}  // namespace arc::net
