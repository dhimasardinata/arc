#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>

#include "arc/result.hpp"

namespace arc::mcap {

enum class Op : std::uint8_t {
    header = 0x01U,
    footer = 0x02U,
    schema = 0x03U,
    channel = 0x04U,
    message = 0x05U,
    data_end = 0x0fU,
};

struct Meta {
    std::string_view key{};
    std::string_view value{};
};

struct Schema {
    std::uint16_t id{};
    std::string_view name{};
    std::string_view encoding{};
    std::span<const std::uint8_t> data{};
};

struct Channel {
    std::uint16_t id{};
    std::uint16_t schema_id{};
    std::string_view topic{};
    std::string_view message_encoding{};
    std::span<const Meta> metadata{};
};

struct Message {
    std::uint16_t channel{};
    std::uint32_t sequence{};
    std::uint64_t log_time{};
    std::uint64_t publish_time{};
    std::span<const std::uint8_t> data{};
};

struct Footer {
    std::uint64_t summary_start{};
    std::uint64_t summary_offset_start{};
    std::uint32_t summary_crc{};
};

struct Writer {
    static constexpr std::array<std::uint8_t, 8> magic{
        0x89U,
        static_cast<std::uint8_t>('M'),
        static_cast<std::uint8_t>('C'),
        static_cast<std::uint8_t>('A'),
        static_cast<std::uint8_t>('P'),
        static_cast<std::uint8_t>('0'),
        static_cast<std::uint8_t>('\r'),
        static_cast<std::uint8_t>('\n'),
    };
    static constexpr std::size_t record_header_bytes = 9U;

    std::span<std::uint8_t> out{};
    std::size_t pos{};
    bool started{};
    bool finished{};

    [[nodiscard]] Status start(
        const std::string_view profile = {},
        const std::string_view library = "arc") noexcept
    {
        if (started || finished || !valid(out) || !fits_string(profile) ||
            !fits_string(library) || !fits_u64(str_bytes(profile), str_bytes(library))) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        ARC_CHECK(write_magic());
        ARC_CHECK(record(Op::header, str_bytes(profile) + str_bytes(library)));
        write_string(profile);
        write_string(library);
        started = true;
        return ok();
    }

    [[nodiscard]] Status schema(const Schema value) noexcept
    {
        if (!started || finished || value.id == 0U ||
            !fits_string(value.name) || !fits_string(value.encoding) || !fits_bytes(value.data) ||
            !fits_u64(2U, str_bytes(value.name), str_bytes(value.encoding), bytes_field(value.data.size()))) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const auto body = 2U + str_bytes(value.name) + str_bytes(value.encoding) + bytes_field(value.data.size());
        ARC_CHECK(record(Op::schema, body));
        write_u16(value.id);
        write_string(value.name);
        write_string(value.encoding);
        write_bytes_field(value.data);
        return ok();
    }

    [[nodiscard]] Status channel(const Channel value) noexcept
    {
        if (!started || finished ||
            !fits_string(value.topic) || !fits_string(value.message_encoding) || !valid(value.metadata)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto meta = map_payload_bytes(value.metadata);
        if (!meta) {
            return Status{fail(meta.error())};
        }
        if (!fits_u64(4U, str_bytes(value.topic), str_bytes(value.message_encoding), 4U, *meta)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const auto body = 4U + str_bytes(value.topic) + str_bytes(value.message_encoding) + 4U + *meta;
        ARC_CHECK(record(Op::channel, body));
        write_u16(value.id);
        write_u16(value.schema_id);
        write_string(value.topic);
        write_string(value.message_encoding);
        write_u32(static_cast<std::uint32_t>(*meta));
        for (const auto item : value.metadata) {
            write_string(item.key);
            write_string(item.value);
        }
        return ok();
    }

    [[nodiscard]] Status message(const Message value) noexcept
    {
        if (!started || finished || !valid(value.data) || !fits_u64(22U, value.data.size())) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        ARC_CHECK(record(Op::message, 22U + value.data.size()));
        write_u16(value.channel);
        write_u32(value.sequence);
        write_u64(value.log_time);
        write_u64(value.publish_time);
        write_raw(value.data);
        return ok();
    }

    [[nodiscard]] Status finish(
        const std::uint32_t data_crc = 0U,
        const Footer footer = {}) noexcept
    {
        if (!started || finished) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }

        constexpr auto tail_bytes = record_header_bytes + 4U + record_header_bytes + 20U + magic.size();
        ARC_CHECK(reserve(tail_bytes));
        ARC_CHECK(record(Op::data_end, 4U));
        write_u32(data_crc);
        ARC_CHECK(record(Op::footer, 20U));
        write_u64(footer.summary_start);
        write_u64(footer.summary_offset_start);
        write_u32(footer.summary_crc);
        ARC_CHECK(write_magic());
        finished = true;
        return ok();
    }

    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept
    {
        return out.first(pos);
    }

private:
    [[nodiscard]] static constexpr bool valid(const std::span<std::uint8_t> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    template <typename T>
    [[nodiscard]] static constexpr bool valid(const std::span<T> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static constexpr bool valid(const std::string_view value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static constexpr bool fits_u32(const std::size_t value) noexcept
    {
        return value <= std::numeric_limits<std::uint32_t>::max();
    }

    template <typename... Rest>
    [[nodiscard]] static constexpr bool fits_u64(
        const std::size_t first,
        const Rest... rest) noexcept
    {
        auto total = first;
        return (((total <= std::numeric_limits<std::uint64_t>::max() - rest) &&
                 (total <= std::numeric_limits<std::size_t>::max() - rest) &&
                 (total += rest, true)) &&
                ...);
    }

    [[nodiscard]] static constexpr bool fits_field(const std::size_t size) noexcept
    {
        return size <= std::numeric_limits<std::uint32_t>::max() &&
            size <= std::numeric_limits<std::size_t>::max() - 4U;
    }

    [[nodiscard]] static constexpr bool fits_string(const std::string_view value) noexcept
    {
        return valid(value) && fits_field(value.size());
    }

    [[nodiscard]] static constexpr bool fits_bytes(const std::span<const std::uint8_t> value) noexcept
    {
        return valid(value) && fits_field(value.size());
    }

    [[nodiscard]] static constexpr std::size_t str_bytes(const std::string_view value) noexcept
    {
        return 4U + value.size();
    }

    [[nodiscard]] static constexpr std::size_t bytes_field(const std::size_t size) noexcept
    {
        return 4U + size;
    }

    [[nodiscard]] static Result<std::size_t> map_payload_bytes(const std::span<const Meta> entries) noexcept
    {
        auto total = std::size_t{};
        for (const auto item : entries) {
            if (!fits_string(item.key) || !fits_string(item.value) ||
                !fits_u64(total, str_bytes(item.key), str_bytes(item.value))) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            total += str_bytes(item.key) + str_bytes(item.value);
        }
        if (!fits_u32(total)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return total;
    }

    [[nodiscard]] Status reserve(const std::size_t bytes) const noexcept
    {
        if (bytes > out.size() || pos > out.size() - bytes) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }
        return ok();
    }

    [[nodiscard]] Status write_magic() noexcept
    {
        ARC_CHECK(reserve(magic.size()));
        write_raw(std::span<const std::uint8_t>{magic});
        return ok();
    }

    [[nodiscard]] Status record(
        const Op op,
        const std::size_t body) noexcept
    {
        if (!fits_u64(record_header_bytes, body)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        ARC_CHECK(reserve(record_header_bytes + body));
        out[pos++] = static_cast<std::uint8_t>(op);
        write_u64(static_cast<std::uint64_t>(body));
        return ok();
    }

    void write_u16(const std::uint16_t value) noexcept
    {
        out[pos++] = static_cast<std::uint8_t>(value);
        out[pos++] = static_cast<std::uint8_t>(value >> 8U);
    }

    void write_u32(const std::uint32_t value) noexcept
    {
        for (auto shift = 0U; shift != 32U; shift += 8U) {
            out[pos++] = static_cast<std::uint8_t>(value >> shift);
        }
    }

    void write_u64(const std::uint64_t value) noexcept
    {
        for (auto shift = 0U; shift != 64U; shift += 8U) {
            out[pos++] = static_cast<std::uint8_t>(value >> shift);
        }
    }

    void write_string(const std::string_view value) noexcept
    {
        write_u32(static_cast<std::uint32_t>(value.size()));
        if (!value.empty()) {
            std::memcpy(out.data() + pos, value.data(), value.size());
            pos += value.size();
        }
    }

    void write_bytes_field(const std::span<const std::uint8_t> value) noexcept
    {
        write_u32(static_cast<std::uint32_t>(value.size()));
        write_raw(value);
    }

    void write_raw(const std::span<const std::uint8_t> value) noexcept
    {
        if (!value.empty()) {
            std::memcpy(out.data() + pos, value.data(), value.size());
            pos += value.size();
        }
    }
};

}  // namespace arc::mcap
