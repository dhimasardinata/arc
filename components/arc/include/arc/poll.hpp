#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "lwip/sockets.h"

#include "arc/result.hpp"

namespace arc::net {

inline constexpr std::uint32_t poll_forever = UINT32_MAX;

enum class PollInterest : std::uint8_t {
    none = 0U,
    read = 1U << 0U,
    write = 1U << 1U,
    error = 1U << 2U,
};

[[nodiscard]] constexpr PollInterest operator|(
    const PollInterest lhs,
    const PollInterest rhs) noexcept
{
    return static_cast<PollInterest>(
        static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

[[nodiscard]] constexpr PollInterest operator&(
    const PollInterest lhs,
    const PollInterest rhs) noexcept
{
    return static_cast<PollInterest>(
        static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr PollInterest& operator|=(PollInterest& lhs, const PollInterest rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool any(const PollInterest value) noexcept
{
    return value != PollInterest::none;
}

struct PollItem {
    int sock{-1};
    PollInterest want{PollInterest::read};
    PollInterest ready{PollInterest::none};
    void* user{};

    [[nodiscard]] constexpr bool readable() const noexcept
    {
        return any(ready & PollInterest::read);
    }

    [[nodiscard]] constexpr bool writable() const noexcept
    {
        return any(ready & PollInterest::write);
    }

    [[nodiscard]] constexpr bool failed() const noexcept
    {
        return any(ready & PollInterest::error);
    }
};

struct Poll {
    [[nodiscard]] static Result<std::size_t> wait(
        const std::span<PollItem> items,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        fd_set read_set;
        fd_set write_set;
        fd_set error_set;
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);

        int max_sock = -1;
        for (auto& item : items) {
            item.ready = PollInterest::none;
            if (item.sock < 0 || !any(item.want)) {
                continue;
            }
#if defined(FD_SETSIZE)
            if (item.sock >= FD_SETSIZE) {
                return fail(ESP_ERR_INVALID_ARG);
            }
#endif

            if (any(item.want & PollInterest::read)) {
                FD_SET(item.sock, &read_set);
            }
            if (any(item.want & PollInterest::write)) {
                FD_SET(item.sock, &write_set);
            }
            FD_SET(item.sock, &error_set);

            if (item.sock > max_sock) {
                max_sock = item.sock;
            }
        }

        if (max_sock < 0) {
            return std::size_t{};
        }

        timeval timeout{};
        timeval* timeout_ptr = nullptr;
        if (timeout_ms != poll_forever) {
            timeout.tv_sec = static_cast<decltype(timeout.tv_sec)>(timeout_ms / 1000U);
            timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>((timeout_ms % 1000U) * 1000U);
            timeout_ptr = &timeout;
        }

        const auto selected = select(max_sock + 1, &read_set, &write_set, &error_set, timeout_ptr);
        if (selected < 0) {
            return fail(last_error());
        }
        if (selected == 0) {
            return std::size_t{};
        }

        std::size_t ready_count{};
        for (auto& item : items) {
            if (item.sock < 0 || !any(item.want)) {
                continue;
            }

            PollInterest ready{};
            if (FD_ISSET(item.sock, &read_set)) {
                ready |= PollInterest::read;
            }
            if (FD_ISSET(item.sock, &write_set)) {
                ready |= PollInterest::write;
            }
            if (FD_ISSET(item.sock, &error_set)) {
                ready |= PollInterest::error;
            }

            item.ready = ready;
            if (any(ready)) {
                ++ready_count;
            }
        }

        return ready_count;
    }

private:
    [[nodiscard]] static esp_err_t last_error() noexcept
    {
        switch (errno) {
            case 0:
                return ESP_FAIL;
            case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
            case ETIMEDOUT:
                return ESP_ERR_TIMEOUT;
            case EINVAL:
            case EBADF:
                return ESP_ERR_INVALID_ARG;
            case ENOMEM:
                return ESP_ERR_NO_MEM;
            default:
                return ESP_FAIL;
        }
    }
};

}  // namespace arc::net
