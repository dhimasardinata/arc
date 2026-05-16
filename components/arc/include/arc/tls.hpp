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
#include "arc/tcp.hpp"
#include "arc/uri.hpp"

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

    [[nodiscard]] static Result<Tls> dial(
        const UriView& uri,
        const std::span<char> host,
        const esp_tls_cfg_t& cfg,
        const std::uint16_t default_port = 443U) noexcept
    {
        if (!secure_scheme(uri)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto endpoint = Uri::endpoint(uri, default_port);
        if (!endpoint) {
            return fail(endpoint.error());
        }

        const auto copied = Uri::copy_host(host, uri);
        if (!copied) {
            return fail(copied.error());
        }
        return dial(copied->data(), endpoint->port, cfg);
    }

    [[nodiscard]] static Result<Tls> dial(
        const char* const uri,
        const std::span<char> host,
        const esp_tls_cfg_t& cfg,
        const std::uint16_t default_port = 443U) noexcept
    {
        const auto parsed = Uri::parse(uri);
        if (!parsed) {
            return fail(parsed.error());
        }
        return dial(*parsed, host, cfg, default_port);
    }

    template <std::size_t N>
    [[nodiscard]] static Result<Tls> dial_ca(
        const char* const host,
        const std::uint16_t port,
        const char (&cacert_pem)[N]) noexcept
    {
        static_assert(N > 1U, "TLS CA PEM must not be empty");
        esp_tls_cfg_t cfg{};
        cfg.cacert_buf = reinterpret_cast<const unsigned char*>(cacert_pem);
        cfg.cacert_bytes = N;
        return dial(host, port, cfg);
    }

    template <std::size_t N>
    [[nodiscard]] static Result<Tls> dial_ca(
        const UriView& uri,
        const std::span<char> host,
        const char (&cacert_pem)[N],
        const std::uint16_t default_port = 443U) noexcept
    {
        static_assert(N > 1U, "TLS CA PEM must not be empty");
        esp_tls_cfg_t cfg{};
        cfg.cacert_buf = reinterpret_cast<const unsigned char*>(cacert_pem);
        cfg.cacert_bytes = N;
        return dial(uri, host, cfg, default_port);
    }

    template <std::size_t N>
    [[nodiscard]] static Result<Tls> dial_ca(
        const char* const uri,
        const std::span<char> host,
        const char (&cacert_pem)[N],
        const std::uint16_t default_port = 443U) noexcept
    {
        static_assert(N > 1U, "TLS CA PEM must not be empty");
        esp_tls_cfg_t cfg{};
        cfg.cacert_buf = reinterpret_cast<const unsigned char*>(cacert_pem);
        cfg.cacert_bytes = N;
        return dial(uri, host, cfg, default_port);
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

    [[nodiscard]] Result<int> sockfd() const noexcept
    {
        if (handle_ == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        int sock = -1;
        const auto err = esp_tls_get_conn_sockfd(handle_, &sock);
        if (err != ESP_OK) {
            return fail(err);
        }
        return sock;
    }

    [[nodiscard]] esp_err_t nonblocking(const bool enable = true) noexcept
    {
        const auto sock = sockfd();
        if (!sock) {
            return sock.error();
        }
        return Tcp::set_nonblocking(*sock, enable);
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
    [[nodiscard]] static bool secure_scheme(const UriView& uri) noexcept
    {
        return !uri.absolute() || Uri::scheme_is(uri, "tls") || Uri::scheme_is(uri, "https") ||
            Uri::scheme_is(uri, "wss") || Uri::scheme_is(uri, "mqtts");
    }

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
