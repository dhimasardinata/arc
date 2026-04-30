#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <cstdio>
#include <span>
#include <type_traits>
#include <utility>

#include "esp_err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "arc/ip.hpp"
#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept TcpBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct Tcp {
    constexpr Tcp() noexcept = default;

    explicit Tcp(const int sock) noexcept
        : sock_(sock)
    {
    }

    Tcp(const Tcp&) = delete;
    Tcp& operator=(const Tcp&) = delete;

    Tcp(Tcp&& other) noexcept
        : sock_(std::exchange(other.sock_, -1))
    {
    }

    Tcp& operator=(Tcp&& other) noexcept
    {
        if (this != &other) {
            close();
            sock_ = std::exchange(other.sock_, -1);
        }
        return *this;
    }

    ~Tcp()
    {
        close();
    }

    [[nodiscard]] static Result<Tcp> dial(
        const char* const host,
        const std::uint16_t port,
        const std::uint32_t timeout_ms = 1000U,
        const IpFamily family = IpFamily::any) noexcept
    {
        if (host == nullptr || port == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        char service[8];
        static_cast<void>(std::snprintf(service, sizeof(service), "%u", static_cast<unsigned>(port)));

        addrinfo hints{};
        hints.ai_family = socket_family(family);
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* res = nullptr;
        const auto gai = getaddrinfo(host, service, &hints, &res);
        if (gai != 0 || res == nullptr) {
            return fail(ESP_FAIL);
        }

        esp_err_t last = ESP_FAIL;
        for (auto* it = res; it != nullptr; it = it->ai_next) {
            const int sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
            if (sock < 0) {
                last = last_error();
                continue;
            }

            const auto timeout = set_timeout(sock, timeout_ms);
            if (timeout != ESP_OK) {
                close_sock(sock);
                last = timeout;
                continue;
            }

            if (::connect(sock, it->ai_addr, it->ai_addrlen) == 0) {
                freeaddrinfo(res);
                return Tcp{sock};
            }

            last = last_error();
            close_sock(sock);
        }

        freeaddrinfo(res);
        return fail(last);
    }

    [[nodiscard]] static Result<Tcp> dial(
        const char* const host,
        const std::uint16_t port,
        const IpFamily family) noexcept
    {
        return dial(host, port, 1000U, family);
    }

    [[nodiscard]] Result<std::size_t> send(const void* const data, const std::size_t bytes) noexcept
    {
        if (sock_ < 0 || data == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto sent = ::send(sock_, data, bytes, 0);
        if (sent < 0) {
            return fail(last_error());
        }
        return static_cast<std::size_t>(sent);
    }

    template <typename T, std::size_t Extent>
        requires TcpBytes<T>
    [[nodiscard]] Result<std::size_t> send(const std::span<T, Extent> data) noexcept
    {
        return send(data.data(), data.size_bytes());
    }

    [[nodiscard]] Status send_all(const void* data, std::size_t bytes) noexcept
    {
        if (sock_ < 0 || data == nullptr) {
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
        requires TcpBytes<T>
    [[nodiscard]] Status send_all(const std::span<T, Extent> data) noexcept
    {
        return send_all(data.data(), data.size_bytes());
    }

    [[nodiscard]] Result<std::size_t> recv(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        if (sock_ < 0 || data == nullptr) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto err = set_recv_timeout(sock_, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }

        const auto got = ::recv(sock_, data, bytes, 0);
        if (got < 0) {
            return fail(last_error());
        }
        return static_cast<std::size_t>(got);
    }

    template <typename T, std::size_t Extent>
        requires(TcpBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] Result<std::size_t> recv(
        const std::span<T, Extent> data,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        return recv(data.data(), data.size_bytes(), timeout_ms);
    }

    [[nodiscard]] esp_err_t nonblocking(const bool enable = true) noexcept
    {
        if (sock_ < 0) {
            return ESP_ERR_INVALID_STATE;
        }
        return set_nonblocking(sock_, enable);
    }

    [[nodiscard]] static esp_err_t set_nonblocking(const int sock, const bool enable = true) noexcept
    {
        if (sock < 0) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto flags = ::fcntl(sock, F_GETFL, 0);
        if (flags < 0) {
            return last_error();
        }

        const auto next = enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        if (::fcntl(sock, F_SETFL, next) != 0) {
            return last_error();
        }
        return ESP_OK;
    }

    void close() noexcept
    {
        if (sock_ >= 0) {
            close_sock(std::exchange(sock_, -1));
        }
    }

    [[nodiscard]] int native() const noexcept
    {
        return sock_;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return sock_ >= 0;
    }

private:
    [[nodiscard]] static timeval make_timeout(const std::uint32_t timeout_ms) noexcept
    {
        timeval timeout{};
        timeout.tv_sec = static_cast<decltype(timeout.tv_sec)>(timeout_ms / 1000U);
        timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>((timeout_ms % 1000U) * 1000U);
        return timeout;
    }

    [[nodiscard]] static esp_err_t set_recv_timeout(
        const int sock,
        const std::uint32_t timeout_ms) noexcept
    {
        const auto timeout = make_timeout(timeout_ms);

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
            return last_error();
        }
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t set_send_timeout(
        const int sock,
        const std::uint32_t timeout_ms) noexcept
    {
        const auto timeout = make_timeout(timeout_ms);

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
            return last_error();
        }
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t set_timeout(
        const int sock,
        const std::uint32_t timeout_ms) noexcept
    {
        const auto recv = set_recv_timeout(sock, timeout_ms);
        if (recv != ESP_OK) {
            return recv;
        }
        return set_send_timeout(sock, timeout_ms);
    }

    static void close_sock(const int sock) noexcept
    {
        static_cast<void>(::shutdown(sock, SHUT_RDWR));
        static_cast<void>(::close(sock));
    }

    [[nodiscard]] static esp_err_t last_error() noexcept
    {
        switch (errno) {
            case 0:
                return ESP_FAIL;
            case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
            case EINPROGRESS:
            case ETIMEDOUT:
                return ESP_ERR_TIMEOUT;
            case EBADF:
            case EINVAL:
                return ESP_ERR_INVALID_ARG;
            case ENOMEM:
                return ESP_ERR_NO_MEM;
            default:
                return ESP_FAIL;
        }
    }

    int sock_{-1};
};

}  // namespace arc::net
