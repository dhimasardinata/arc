#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept StreamBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <typename T>
concept ByteStream = requires(T& stream, const void* in, void* out, std::size_t bytes) {
    { stream.send_all(in, bytes) } -> std::same_as<Status>;
    { stream.recv(out, bytes) } -> std::same_as<Result<std::size_t>>;
};

struct Stream {
    template <ByteStream Io>
    [[nodiscard]] static Status write(Io& io, const void* const data, const std::size_t bytes) noexcept
    {
        if (data == nullptr && bytes != 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (bytes == 0U) {
            return ok();
        }
        return io.send_all(data, bytes);
    }

    template <ByteStream Io, typename T, std::size_t Extent>
        requires StreamBytes<T>
    [[nodiscard]] static Status write(Io& io, const std::span<T, Extent> data) noexcept
    {
        return write(io, data.data(), data.size_bytes());
    }

    template <ByteStream Io>
    [[nodiscard]] static Status read_exact(Io& io, void* const data, std::size_t bytes) noexcept
    {
        if (data == nullptr && bytes != 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto* cursor = static_cast<std::uint8_t*>(data);
        while (bytes != 0U) {
            const auto got = io.recv(cursor, bytes);
            if (!got) {
                return Status{fail(got.error())};
            }
            if (*got == 0U) {
                return Status{fail(ESP_FAIL)};
            }

            cursor += *got;
            bytes -= *got;
        }

        return ok();
    }

    template <ByteStream Io, typename T, std::size_t Extent>
        requires(StreamBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static Status read_exact(Io& io, const std::span<T, Extent> data) noexcept
    {
        return read_exact(io, data.data(), data.size_bytes());
    }

    template <ByteStream Io>
    [[nodiscard]] static Status write_frame16(Io& io, const void* const data, const std::size_t bytes) noexcept
    {
        if ((data == nullptr && bytes != 0U) || bytes > 0xffffU) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const std::array<std::uint8_t, 2> header{
            static_cast<std::uint8_t>(bytes >> 8U),
            static_cast<std::uint8_t>(bytes),
        };
        auto sent = io.send_all(header.data(), header.size());
        if (!sent) {
            return sent;
        }
        if (bytes == 0U) {
            return ok();
        }
        return io.send_all(data, bytes);
    }

    template <ByteStream Io, typename T, std::size_t Extent>
        requires StreamBytes<T>
    [[nodiscard]] static Status write_frame16(Io& io, const std::span<T, Extent> data) noexcept
    {
        return write_frame16(io, data.data(), data.size_bytes());
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> frame16(
        std::span<std::uint8_t> out,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        if ((data == nullptr && bytes != 0U) || bytes > 0xffffU) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (out.size() < bytes + 2U) {
            return fail(ESP_ERR_NO_MEM);
        }

        out[0] = static_cast<std::uint8_t>(bytes >> 8U);
        out[1] = static_cast<std::uint8_t>(bytes);
        if (bytes != 0U) {
            std::memcpy(out.data() + 2U, data, bytes);
        }
        return out.first(bytes + 2U);
    }

    template <typename T, std::size_t OutExtent, std::size_t InExtent>
        requires StreamBytes<T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> frame16(
        const std::span<std::uint8_t, OutExtent> out,
        const std::span<T, InExtent> data) noexcept
    {
        return frame16(out, data.data(), data.size_bytes());
    }

    template <ByteStream Io>
    [[nodiscard]] static Result<std::size_t> read_frame16(Io& io, void* const data, const std::size_t cap) noexcept
    {
        std::array<std::uint8_t, 2> header{};
        auto read = read_exact(io, header.data(), header.size());
        if (!read) {
            return fail(read.error());
        }

        const auto bytes = (static_cast<std::size_t>(header[0]) << 8U) | header[1];
        if (bytes > cap) {
            return fail(ESP_ERR_NO_MEM);
        }

        read = read_exact(io, data, bytes);
        if (!read) {
            return fail(read.error());
        }
        return bytes;
    }

    template <ByteStream Io, typename T, std::size_t Extent>
        requires(StreamBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static Result<std::size_t> read_frame16(Io& io, const std::span<T, Extent> data) noexcept
    {
        return read_frame16(io, data.data(), data.size_bytes());
    }
};

}  // namespace arc::net
