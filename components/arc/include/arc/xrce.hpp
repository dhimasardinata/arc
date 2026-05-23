#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "arc/result.hpp"

namespace arc::xrce {

inline constexpr std::uint8_t stream_none = 0x00U;
inline constexpr std::uint8_t stream_best = 0x01U;
inline constexpr std::uint8_t stream_reliable = 0x80U;
inline constexpr std::uint8_t little = 0x01U;

enum class Sub : std::uint8_t {
    create_client = 0x00U,
    write_data = 0x07U,
    data = 0x0FU,
    acknack = 0x10U,
    heartbeat = 0x11U,
};

struct Key {
    std::array<std::uint8_t, 4> bytes{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return bytes[0] != 0U || bytes[1] != 0U || bytes[2] != 0U || bytes[3] != 0U;
    }
};

struct Header {
    std::uint8_t session{};
    std::uint8_t stream{stream_none};
    std::uint16_t seq{};
    Key key{};
    bool keyed{};

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return 4U + (keyed ? key.bytes.size() : 0U);
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return !keyed || key.valid();
    }
};

struct HeaderView {
    Header header{};
    std::size_t next{};
};

struct Submsg {
    Sub id{};
    std::uint8_t flags{little};
    std::span<const std::uint8_t> payload{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return (payload.empty() || payload.data() != nullptr) && payload.size() <= 0xffffU;
    }
};

struct SubView {
    Sub id{};
    std::uint8_t flags{};
    std::span<const std::uint8_t> payload{};
    std::size_t next{};
};

[[nodiscard]] constexpr bool reliable(const std::uint8_t stream) noexcept
{
    return stream >= stream_reliable;
}

[[nodiscard]] constexpr bool best(const std::uint8_t stream) noexcept
{
    return stream > stream_none && stream < stream_reliable;
}

namespace detail {

template <typename Byte>
[[nodiscard]] constexpr bool valid(const std::span<Byte> data) noexcept
{
    return data.empty() || data.data() != nullptr;
}

inline void put16(std::uint8_t* out, const std::uint16_t value, const bool le) noexcept
{
    if (le) {
        out[0] = static_cast<std::uint8_t>(value);
        out[1] = static_cast<std::uint8_t>(value >> 8U);
    } else {
        out[0] = static_cast<std::uint8_t>(value >> 8U);
        out[1] = static_cast<std::uint8_t>(value);
    }
}

[[nodiscard]] inline std::uint16_t get16(const std::uint8_t* data, const bool le) noexcept
{
    if (le) {
        return static_cast<std::uint16_t>(data[0] | (static_cast<std::uint16_t>(data[1]) << 8U));
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[0]) << 8U) | data[1]);
}

}  // namespace detail

struct Writer {
    std::span<std::uint8_t> out{};
    std::size_t pos{};

    [[nodiscard]] constexpr std::size_t remain() const noexcept
    {
        return pos <= out.size() ? out.size() - pos : 0U;
    }

    [[nodiscard]] Status header(const Header value) noexcept
    {
        if (!detail::valid(out) || !value.valid() || remain() < value.size()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        out[pos++] = value.session;
        out[pos++] = value.stream;
        detail::put16(out.data() + pos, value.seq, true);
        pos += 2U;
        if (value.keyed) {
            std::memcpy(out.data() + pos, value.key.bytes.data(), value.key.bytes.size());
            pos += value.key.bytes.size();
        }
        return ok();
    }

    [[nodiscard]] Result<std::span<std::uint8_t>> reserve(
        const Sub id,
        const std::uint8_t flags,
        const std::size_t bytes) noexcept
    {
        if (!detail::valid(out) || bytes > 0xffffU || remain() < 4U + bytes) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        out[pos++] = static_cast<std::uint8_t>(id);
        out[pos++] = flags;
        detail::put16(out.data() + pos, static_cast<std::uint16_t>(bytes), (flags & little) != 0U);
        pos += 2U;
        auto payload = out.subspan(pos, bytes);
        pos += bytes;
        return payload;
    }

    [[nodiscard]] Status sub(const Submsg value) noexcept
    {
        if (!value.valid()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        auto payload = reserve(value.id, value.flags, value.payload.size());
        if (!payload) {
            return Status{fail(payload.error())};
        }
        if (!value.payload.empty()) {
            std::memcpy(payload->data(), value.payload.data(), value.payload.size());
        }
        return ok();
    }

    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept
    {
        return {out.data(), pos};
    }
};

[[nodiscard]] inline Result<HeaderView> read_header(
    const std::span<const std::uint8_t> data,
    const bool keyed = false) noexcept
{
    if (!detail::valid(data) || data.size() < 4U + (keyed ? 4U : 0U)) {
        return fail(ESP_ERR_INVALID_ARG);
    }

    Header header{
        .session = data[0],
        .stream = data[1],
        .seq = detail::get16(data.data() + 2U, true),
        .keyed = keyed,
    };
    std::size_t next = 4U;
    if (keyed) {
        std::memcpy(header.key.bytes.data(), data.data() + next, header.key.bytes.size());
        next += header.key.bytes.size();
        if (!header.key.valid()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
    }
    return HeaderView{.header = header, .next = next};
}

[[nodiscard]] inline Result<SubView> read_sub(
    const std::span<const std::uint8_t> data,
    const std::size_t at = 0U) noexcept
{
    if (!detail::valid(data) || at > data.size() || data.size() - at < 4U) {
        return fail(ESP_ERR_INVALID_ARG);
    }

    const auto id = static_cast<Sub>(data[at]);
    const auto flags = data[at + 1U];
    const auto size = detail::get16(data.data() + at + 2U, (flags & little) != 0U);
    const auto payload = at + 4U;
    const auto next = payload + size;
    if (next > data.size()) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return SubView{
        .id = id,
        .flags = flags,
        .payload = data.subspan(payload, size),
        .next = next,
    };
}

}  // namespace arc::xrce
