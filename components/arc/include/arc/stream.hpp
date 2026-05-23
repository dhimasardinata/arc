#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <system_error>
#include <type_traits>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept StreamBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

[[nodiscard]] constexpr bool valid_text(const std::string_view value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

}  // namespace detail

template <typename T>
concept ByteStream = requires(T& stream, const void* in, void* out, std::size_t bytes) {
    { stream.send_all(in, bytes) } -> std::same_as<Status>;
    { stream.recv(out, bytes) } -> std::same_as<Result<std::size_t>>;
};

struct AnyStream {
    using SendFn = Status (*)(void*, const void*, std::size_t) noexcept;
    using RecvFn = Result<std::size_t> (*)(void*, void*, std::size_t) noexcept;

    void* ctx{};
    SendFn send_fn{};
    RecvFn recv_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return send_fn != nullptr && recv_fn != nullptr;
    }

    template <ByteStream Io>
    [[nodiscard]] static constexpr AnyStream bind(Io& io) noexcept
    {
        return AnyStream{
            .ctx = &io,
            .send_fn = &send_object<Io>,
            .recv_fn = &recv_object<Io>,
        };
    }

    [[nodiscard]] Status send_all(const void* const data, const std::size_t bytes) noexcept
    {
        if (send_fn == nullptr || ctx == nullptr || (data == nullptr && bytes != 0U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (bytes == 0U) {
            return ok();
        }
        return send_fn(ctx, data, bytes);
    }

    [[nodiscard]] Result<std::size_t> recv(void* const data, const std::size_t bytes) noexcept
    {
        if (recv_fn == nullptr || ctx == nullptr || (data == nullptr && bytes != 0U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (bytes == 0U) {
            return 0U;
        }
        return recv_fn(ctx, data, bytes);
    }

private:
    template <ByteStream Io>
    [[nodiscard]] static Status send_object(
        void* const ctx,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return static_cast<Io*>(ctx)->send_all(data, bytes);
    }

    template <ByteStream Io>
    [[nodiscard]] static Result<std::size_t> recv_object(
        void* const ctx,
        void* const data,
        const std::size_t bytes) noexcept
    {
        return static_cast<Io*>(ctx)->recv(data, bytes);
    }
};

static_assert(ByteStream<AnyStream>);

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
        if (!detail::valid_span(out)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (out.size() < bytes + 2U) {
            return fail(ESP_ERR_NO_MEM);
        }

        if (bytes != 0U) {
            std::memmove(out.data() + 2U, data, bytes);
        }
        out[0] = static_cast<std::uint8_t>(bytes >> 8U);
        out[1] = static_cast<std::uint8_t>(bytes);
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
        if (data == nullptr && cap != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

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

struct Rtp {
    static constexpr std::size_t header_bytes = 12U;

    [[nodiscard]] static Result<std::span<const std::uint8_t>> packet(
        const std::span<std::uint8_t> out,
        const std::span<const std::uint8_t> payload,
        const std::uint16_t sequence,
        const std::uint32_t timestamp,
        const std::uint32_t ssrc,
        const std::uint8_t payload_type = 96U,
        const bool marker = false) noexcept
    {
        if (payload_type > 0x7fU) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (!detail::valid_span(out) || !detail::valid_span(payload)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (out.size() < header_bytes || payload.size() > out.size() - header_bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        const auto bytes = header_bytes + payload.size();
        if (!payload.empty()) {
            std::memmove(out.data() + header_bytes, payload.data(), payload.size());
        }
        out[0] = 0x80U;
        out[1] = static_cast<std::uint8_t>((marker ? 0x80U : 0U) | payload_type);
        out[2] = static_cast<std::uint8_t>(sequence >> 8U);
        out[3] = static_cast<std::uint8_t>(sequence);
        out[4] = static_cast<std::uint8_t>(timestamp >> 24U);
        out[5] = static_cast<std::uint8_t>(timestamp >> 16U);
        out[6] = static_cast<std::uint8_t>(timestamp >> 8U);
        out[7] = static_cast<std::uint8_t>(timestamp);
        out[8] = static_cast<std::uint8_t>(ssrc >> 24U);
        out[9] = static_cast<std::uint8_t>(ssrc >> 16U);
        out[10] = static_cast<std::uint8_t>(ssrc >> 8U);
        out[11] = static_cast<std::uint8_t>(ssrc);
        return out.first(bytes);
    }

    template <ByteStream Io>
    [[nodiscard]] static Status write(
        Io& io,
        const std::span<const std::uint8_t> payload,
        const std::uint16_t sequence,
        const std::uint32_t timestamp,
        const std::uint32_t ssrc,
        const std::uint8_t payload_type = 96U,
        const bool marker = false) noexcept
    {
        if (!detail::valid_span(payload)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        std::array<std::uint8_t, header_bytes> header{};
        auto framed = packet(
            std::span(header),
            std::span<const std::uint8_t>{},
            sequence,
            timestamp,
            ssrc,
            payload_type,
            marker);
        if (!framed) {
            return Status{fail(framed.error())};
        }
        auto sent = io.send_all(header.data(), header.size());
        if (!sent) {
            return sent;
        }
        return payload.empty() ? ok() : io.send_all(payload.data(), payload.size());
    }
};

struct Mjpeg {
    [[nodiscard]] static Result<std::span<const std::uint8_t>> part(
        const std::span<std::uint8_t> out,
        const std::span<const std::uint8_t> jpeg,
        const std::string_view boundary = "arc") noexcept
    {
        if (!detail::valid_span(out) || !detail::valid_span(jpeg) || !detail::valid_text(boundary)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (boundary.empty() || boundary.size() > 64U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto* cursor = out.data();
        auto remaining = out.size();
        const auto append = [&](const std::string_view text) noexcept -> bool {
            if (remaining < text.size()) {
                return false;
            }
            std::memcpy(cursor, text.data(), text.size());
            cursor += text.size();
            remaining -= text.size();
            return true;
        };

        if (!append("--") || !append(boundary) ||
            !append("\r\nContent-Type: image/jpeg\r\nContent-Length: ")) {
            return fail(ESP_ERR_NO_MEM);
        }

        std::array<char, 20> digits{};
        const auto [ptr, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), jpeg.size());
        if (ec != std::errc{}) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (!append(std::string_view{digits.data(), static_cast<std::size_t>(ptr - digits.data())}) ||
            !append("\r\n\r\n")) {
            return fail(ESP_ERR_NO_MEM);
        }
        if (jpeg.size() > remaining || remaining - jpeg.size() < 2U) {
            return fail(ESP_ERR_NO_MEM);
        }
        if (!jpeg.empty()) {
            std::memcpy(cursor, jpeg.data(), jpeg.size());
            cursor += jpeg.size();
            remaining -= jpeg.size();
        }
        if (!append("\r\n")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(static_cast<std::size_t>(cursor - out.data()));
    }

    template <ByteStream Io>
    [[nodiscard]] static Status write_part(
        Io& io,
        const std::span<const std::uint8_t> jpeg,
        const std::string_view boundary = "arc") noexcept
    {
        if (!detail::valid_span(jpeg) || !detail::valid_text(boundary) || boundary.empty() || boundary.size() > 64U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        std::array<std::uint8_t, 128> header{};
        auto* cursor = header.data();
        auto remaining = header.size();
        const auto append = [&](const std::string_view text) noexcept -> bool {
            if (remaining < text.size()) {
                return false;
            }
            std::memcpy(cursor, text.data(), text.size());
            cursor += text.size();
            remaining -= text.size();
            return true;
        };

        std::array<char, 20> digits{};
        const auto [ptr, ec] = std::to_chars(digits.data(), digits.data() + digits.size(), jpeg.size());
        if (ec != std::errc{} ||
            !append("--") ||
            !append(boundary) ||
            !append("\r\nContent-Type: image/jpeg\r\nContent-Length: ") ||
            !append(std::string_view{digits.data(), static_cast<std::size_t>(ptr - digits.data())}) ||
            !append("\r\n\r\n")) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto sent = io.send_all(header.data(), static_cast<std::size_t>(cursor - header.data()));
        if (!sent) {
            return sent;
        }
        sent = jpeg.empty() ? ok() : io.send_all(jpeg.data(), jpeg.size());
        if (!sent) {
            return sent;
        }
        constexpr std::array<std::uint8_t, 2> crlf{'\r', '\n'};
        return io.send_all(crlf.data(), crlf.size());
    }
};

}  // namespace arc::net
