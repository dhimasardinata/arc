#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept CoapBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

enum class CoapType : std::uint8_t {
    confirmable = 0U,
    nonconfirmable = 1U,
    acknowledgement = 2U,
    reset = 3U,
};

enum class CoapCode : std::uint8_t {
    empty = 0x00U,
    get = 0x01U,
    post = 0x02U,
    put = 0x03U,
    del = 0x04U,
    created = 0x41U,
    deleted = 0x42U,
    valid = 0x43U,
    changed = 0x44U,
    content = 0x45U,
    bad_request = 0x80U,
    not_found = 0x84U,
    not_allowed = 0x85U,
    internal_error = 0xA0U,
};

enum class CoapOptionNumber : std::uint16_t {
    uri_host = 3U,
    etag = 4U,
    none_match = 5U,
    observe = 6U,
    uri_port = 7U,
    location_path = 8U,
    uri_path = 11U,
    content_format = 12U,
    max_age = 14U,
    uri_query = 15U,
    accept = 17U,
    location_query = 20U,
    block2 = 23U,
    block1 = 27U,
    proxy_uri = 35U,
    proxy_scheme = 39U,
    size1 = 60U,
};

struct CoapOption {
    std::uint16_t number{};
    std::span<const std::uint8_t> value{};
};

struct CoapOptionView {
    std::uint16_t number{};
    std::span<const std::uint8_t> value{};
};

struct CoapMessage {
    CoapType type{};
    std::uint8_t code{};
    std::uint16_t id{};
    std::span<const std::uint8_t> token{};
    std::span<const std::uint8_t> options{};
    std::span<const std::uint8_t> payload{};
    std::size_t bytes{};
};

struct Coap {
    [[nodiscard]] static constexpr std::uint8_t code(
        const std::uint8_t klass,
        const std::uint8_t detail) noexcept
    {
        return static_cast<std::uint8_t>((klass << 5U) | (detail & 0x1fU));
    }

    [[nodiscard]] static constexpr CoapOption option(
        const std::uint16_t number,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return CoapOption{
            .number = number,
            .value = std::span<const std::uint8_t>{
                static_cast<const std::uint8_t*>(data),
                bytes,
            },
        };
    }

    template <typename T, std::size_t Extent>
        requires CoapBytes<T>
    [[nodiscard]] static constexpr CoapOption option(
        const std::uint16_t number,
        const std::span<T, Extent> data) noexcept
    {
        return option(number, data.data(), data.size_bytes());
    }

    [[nodiscard]] static CoapOption text(
        const std::uint16_t number,
        const char* const value) noexcept
    {
        return option(number, value, value != nullptr ? std::strlen(value) : 0U);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> message(
        std::span<std::uint8_t> out,
        const CoapType type,
        const std::uint8_t code,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {},
        const std::span<const CoapOption> options = {},
        const std::span<const std::uint8_t> payload = {}) noexcept
    {
        if (token.size() > 8U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t bytes = 4U + token.size();
        auto previous = std::uint16_t{0U};
        for (const auto& option : options) {
            if ((option.value.size() != 0U && option.value.data() == nullptr) || option.number < previous) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            const auto delta = static_cast<std::size_t>(option.number - previous);
            const auto delta_bytes = extended_bytes(delta);
            const auto length_bytes = extended_bytes(option.value.size());
            if (delta_bytes > 2U || length_bytes > 2U) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            bytes += 1U + delta_bytes + length_bytes + option.value.size();
            previous = option.number;
        }

        if (!payload.empty()) {
            bytes += 1U + payload.size();
        }
        if (out.size() < bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto pos = std::size_t{0U};
        out[pos++] = static_cast<std::uint8_t>((1U << 6U) |
                                               (static_cast<std::uint8_t>(type) << 4U) |
                                               token.size());
        out[pos++] = code;
        pos += write_u16(out, pos, id);

        if (!token.empty()) {
            std::memcpy(out.data() + pos, token.data(), token.size());
            pos += token.size();
        }

        previous = 0U;
        for (const auto& option : options) {
            const auto delta = static_cast<std::size_t>(option.number - previous);
            pos += write_option(out, pos, delta, option.value.size());

            if (!option.value.empty()) {
                std::memcpy(out.data() + pos, option.value.data(), option.value.size());
                pos += option.value.size();
            }
            previous = option.number;
        }

        if (!payload.empty()) {
            out[pos++] = 0xffU;
            std::memcpy(out.data() + pos, payload.data(), payload.size());
            pos += payload.size();
        }
        return out.first(pos);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> ping(
        std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {}) noexcept
    {
        return message(out, CoapType::confirmable, static_cast<std::uint8_t>(CoapCode::empty), id, token);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> reset(
        std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {}) noexcept
    {
        return message(out, CoapType::reset, static_cast<std::uint8_t>(CoapCode::empty), id, token);
    }

    [[nodiscard]] static Result<CoapMessage> parse(const std::span<const std::uint8_t> frame) noexcept
    {
        if (frame.size() < 4U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto version = frame[0] >> 6U;
        const auto token_len = static_cast<std::size_t>(frame[0] & 0x0fU);
        if (version != 1U || token_len > 8U || frame.size() < 4U + token_len) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto options_begin = 4U + token_len;
        auto options_end = options_begin;
        auto number = std::uint16_t{0U};

        while (options_end < frame.size() && frame[options_end] != 0xffU) {
            auto offset = options_end - options_begin;
            const auto option = next(frame.subspan(options_begin), offset, number);
            if (!option) {
                return fail(option.error());
            }
            options_end = options_begin + offset;
        }

        std::span<const std::uint8_t> payload{};
        if (options_end < frame.size()) {
            if (frame[options_end] != 0xffU || options_end + 1U >= frame.size()) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            payload = frame.subspan(options_end + 1U);
        }

        return CoapMessage{
            .type = static_cast<CoapType>((frame[0] >> 4U) & 0x03U),
            .code = frame[1],
            .id = read_u16(frame, 2U),
            .token = frame.subspan(4U, token_len),
            .options = frame.subspan(options_begin, options_end - options_begin),
            .payload = payload,
            .bytes = frame.size(),
        };
    }

    [[nodiscard]] static Result<CoapOptionView> next(
        const std::span<const std::uint8_t> options,
        std::size_t& offset,
        std::uint16_t& number) noexcept
    {
        if (offset >= options.size()) {
            return fail(ESP_ERR_NOT_FOUND);
        }
        if (options[offset] == 0xffU) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto pos = offset;
        const auto byte = options[pos++];
        const auto delta = read_extended(options, pos, static_cast<std::uint8_t>(byte >> 4U));
        if (!delta) {
            return fail(delta.error());
        }

        const auto length = read_extended(options, pos, static_cast<std::uint8_t>(byte & 0x0fU));
        if (!length) {
            return fail(length.error());
        }
        if (options.size() < pos + *length) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        if (*delta > (0xffffU - number)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        number = static_cast<std::uint16_t>(number + *delta);
        const auto value = options.subspan(pos, *length);
        pos += *length;
        offset = pos;
        return CoapOptionView{
            .number = number,
            .value = value,
        };
    }

private:
    [[nodiscard]] static constexpr std::size_t extended_bytes(const std::size_t value) noexcept
    {
        return value < 13U ? 0U : value < 269U ? 1U
            : value < 65805U                   ? 2U
                                               : 3U;
    }

    static std::size_t write_option(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::size_t delta,
        const std::size_t length) noexcept
    {
        const auto delta_nibble = nibble(delta);
        const auto length_nibble = nibble(length);
        auto used = std::size_t{0U};
        out[pos + used++] = static_cast<std::uint8_t>((delta_nibble << 4U) | length_nibble);
        used += write_extended(out, pos + used, delta, delta_nibble);
        used += write_extended(out, pos + used, length, length_nibble);
        return used;
    }

    [[nodiscard]] static constexpr std::uint8_t nibble(const std::size_t value) noexcept
    {
        return value < 13U ? static_cast<std::uint8_t>(value) : value < 269U ? 13U
                                                                             : 14U;
    }

    static std::size_t write_extended(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::size_t value,
        const std::uint8_t nibble) noexcept
    {
        if (nibble < 13U) {
            return 0U;
        }
        if (nibble == 13U) {
            out[pos] = static_cast<std::uint8_t>(value - 13U);
            return 1U;
        }

        out[pos + 0U] = static_cast<std::uint8_t>((value - 269U) >> 8U);
        out[pos + 1U] = static_cast<std::uint8_t>((value - 269U) & 0xffU);
        return 2U;
    }

    [[nodiscard]] static Result<std::size_t> read_extended(
        const std::span<const std::uint8_t> in,
        std::size_t& pos,
        const std::uint8_t nibble) noexcept
    {
        if (nibble < 13U) {
            return static_cast<std::size_t>(nibble);
        }
        if (nibble == 13U) {
            if (in.size() < pos + 1U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            return static_cast<std::size_t>(13U + in[pos++]);
        }
        if (nibble == 14U) {
            if (in.size() < pos + 2U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto value = static_cast<std::size_t>(269U + read_u16(in, pos));
            pos += 2U;
            return value;
        }
        return fail(ESP_ERR_INVALID_ARG);
    }

    static std::size_t write_u16(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::uint16_t value) noexcept
    {
        out[pos + 0U] = static_cast<std::uint8_t>(value >> 8U);
        out[pos + 1U] = static_cast<std::uint8_t>(value & 0xffU);
        return 2U;
    }

    [[nodiscard]] static std::uint16_t read_u16(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(in[pos]) << 8U) | in[pos + 1U]);
    }
};

template <typename Codec>
concept CoapAdapter = requires(
    Codec& codec,
    std::span<std::uint8_t> out,
    std::span<const std::uint8_t> token,
    std::span<const CoapOption> options,
    std::span<const std::uint8_t> payload,
    std::span<const std::uint8_t> frame,
    std::span<const std::uint8_t> option_bytes,
    std::size_t& offset,
    std::uint16_t& number) {
    { codec.message(out, CoapType::confirmable, std::uint8_t{}, std::uint16_t{}, token, options, payload) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.ping(out, std::uint16_t{}, token) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.reset(out, std::uint16_t{}, token) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.parse(frame) } -> std::same_as<Result<CoapMessage>>;
    { codec.next(option_bytes, offset, number) } -> std::same_as<Result<CoapOptionView>>;
};

struct AnyCoap {
    using MessageFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        CoapType,
        std::uint8_t,
        std::uint16_t,
        std::span<const std::uint8_t>,
        std::span<const CoapOption>,
        std::span<const std::uint8_t>) noexcept;
    using EmptyFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        std::uint16_t,
        std::span<const std::uint8_t>) noexcept;
    using ParseFn = Result<CoapMessage> (*)(void*, std::span<const std::uint8_t>) noexcept;
    using NextFn = Result<CoapOptionView> (*)(
        void*,
        std::span<const std::uint8_t>,
        std::size_t&,
        std::uint16_t&) noexcept;

    void* ctx{};
    MessageFn message_fn{};
    EmptyFn ping_fn{};
    EmptyFn reset_fn{};
    ParseFn parse_fn{};
    NextFn next_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return message_fn != nullptr &&
            ping_fn != nullptr &&
            reset_fn != nullptr &&
            parse_fn != nullptr &&
            next_fn != nullptr;
    }

    [[nodiscard]] static constexpr AnyCoap arc() noexcept
    {
        return {
            .ctx = nullptr,
            .message_fn = &arc_message,
            .ping_fn = &arc_ping,
            .reset_fn = &arc_reset,
            .parse_fn = &arc_parse,
            .next_fn = &arc_next,
        };
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static constexpr AnyCoap bind(Codec& codec) noexcept
    {
        return {
            .ctx = &codec,
            .message_fn = &message_object<Codec>,
            .ping_fn = &ping_object<Codec>,
            .reset_fn = &reset_object<Codec>,
            .parse_fn = &parse_object<Codec>,
            .next_fn = &next_object<Codec>,
        };
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> message(
        std::span<std::uint8_t> out,
        const CoapType type,
        const std::uint8_t code,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {},
        const std::span<const CoapOption> options = {},
        const std::span<const std::uint8_t> payload = {}) noexcept
    {
        return message_fn == nullptr ? fail(ESP_ERR_INVALID_STATE)
                                     : message_fn(ctx, out, type, code, id, token, options, payload);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> ping(
        std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {}) noexcept
    {
        return ping_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : ping_fn(ctx, out, id, token);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> reset(
        std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token = {}) noexcept
    {
        return reset_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : reset_fn(ctx, out, id, token);
    }

    [[nodiscard]] Result<CoapMessage> parse(const std::span<const std::uint8_t> frame) noexcept
    {
        return parse_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : parse_fn(ctx, frame);
    }

    [[nodiscard]] Result<CoapOptionView> next(
        const std::span<const std::uint8_t> options,
        std::size_t& offset,
        std::uint16_t& number) noexcept
    {
        return next_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : next_fn(ctx, options, offset, number);
    }

private:
    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_message(
        void*,
        const std::span<std::uint8_t> out,
        const CoapType type,
        const std::uint8_t code,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token,
        const std::span<const CoapOption> options,
        const std::span<const std::uint8_t> payload) noexcept
    {
        return Coap::message(out, type, code, id, token, options, payload);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_ping(
        void*,
        const std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token) noexcept
    {
        return Coap::ping(out, id, token);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_reset(
        void*,
        const std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token) noexcept
    {
        return Coap::reset(out, id, token);
    }

    [[nodiscard]] static Result<CoapMessage> arc_parse(
        void*,
        const std::span<const std::uint8_t> frame) noexcept
    {
        return Coap::parse(frame);
    }

    [[nodiscard]] static Result<CoapOptionView> arc_next(
        void*,
        const std::span<const std::uint8_t> options,
        std::size_t& offset,
        std::uint16_t& number) noexcept
    {
        return Coap::next(options, offset, number);
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> message_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const CoapType type,
        const std::uint8_t code,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token,
        const std::span<const CoapOption> options,
        const std::span<const std::uint8_t> payload) noexcept
    {
        return static_cast<Codec*>(ctx)->message(out, type, code, id, token, options, payload);
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> ping_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token) noexcept
    {
        return static_cast<Codec*>(ctx)->ping(out, id, token);
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> reset_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const std::uint16_t id,
        const std::span<const std::uint8_t> token) noexcept
    {
        return static_cast<Codec*>(ctx)->reset(out, id, token);
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static Result<CoapMessage> parse_object(
        void* const ctx,
        const std::span<const std::uint8_t> frame) noexcept
    {
        return static_cast<Codec*>(ctx)->parse(frame);
    }

    template <CoapAdapter Codec>
    [[nodiscard]] static Result<CoapOptionView> next_object(
        void* const ctx,
        const std::span<const std::uint8_t> options,
        std::size_t& offset,
        std::uint16_t& number) noexcept
    {
        return static_cast<Codec*>(ctx)->next(options, offset, number);
    }
};

}  // namespace arc::net
