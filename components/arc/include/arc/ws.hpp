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
concept WsBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

enum class WsOpcode : std::uint8_t {
    continuation = 0x0U,
    text = 0x1U,
    binary = 0x2U,
    close = 0x8U,
    ping = 0x9U,
    pong = 0xAU,
};

struct WsFrame {
    WsOpcode opcode{};
    std::span<const std::uint8_t> payload{};
    std::size_t bytes{};
    std::uint32_t mask{};
    bool fin{};
    bool rsv1{};
    bool rsv2{};
    bool rsv3{};
    bool masked{};
};

struct WsClose {
    std::uint16_t code{};
    std::span<const std::uint8_t> reason{};
};

struct Ws {
    [[nodiscard]] static Result<std::span<const char>> key(
        std::span<char> out,
        const std::span<const std::uint8_t> nonce) noexcept
    {
        if (nonce.size() != 16U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return base64(out, nonce);
    }

    [[nodiscard]] static Result<std::span<const char>> accept(
        std::span<char> out,
        const char* const key) noexcept
    {
        if (key == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        Sha1 sha{};
        sha.update(reinterpret_cast<const std::uint8_t*>(key), std::strlen(key));
        sha.update(reinterpret_cast<const std::uint8_t*>(guid), sizeof(guid) - 1U);

        std::uint8_t digest[20]{};
        sha.final(digest);
        return base64(out, std::span<const std::uint8_t>{digest, sizeof(digest)});
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> frame(
        std::span<std::uint8_t> out,
        const WsOpcode opcode,
        const void* const data,
        const std::size_t bytes,
        const bool fin = true,
        const std::uint32_t mask = 0U,
        const bool rsv1 = false,
        const bool rsv2 = false,
        const bool rsv3 = false) noexcept
    {
        if ((bytes != 0U && data == nullptr) || !valid_opcode(opcode) ||
            (control(opcode) && (!fin || bytes > 125U))) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto header = header_bytes(bytes, mask != 0U);
        if (out.size() < header + bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto pos = std::size_t{0U};
        out[pos++] =
            static_cast<std::uint8_t>((fin ? 0x80U : 0U) |
                                      (rsv1 ? 0x40U : 0U) |
                                      (rsv2 ? 0x20U : 0U) |
                                      (rsv3 ? 0x10U : 0U) |
                                      static_cast<std::uint8_t>(opcode));
        pos += write_length(out, pos, bytes, mask != 0U);

        if (mask != 0U) {
            pos += write_u32(out, pos, mask);
        }

        const auto* const src = static_cast<const std::uint8_t*>(data);
        for (std::size_t i = 0; i < bytes; ++i) {
            auto byte = src[i];
            if (mask != 0U) {
                byte ^= mask_byte(mask, i);
            }
            out[pos + i] = byte;
        }
        pos += bytes;
        return out.first(pos);
    }

    template <typename T, std::size_t Extent>
        requires WsBytes<T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> binary(
        std::span<std::uint8_t> out,
        const std::span<T, Extent> data,
        const bool fin = true,
        const std::uint32_t mask = 0U) noexcept
    {
        return frame(out, WsOpcode::binary, data.data(), data.size_bytes(), fin, mask);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> text(
        std::span<std::uint8_t> out,
        const char* const text,
        const bool fin = true,
        const std::uint32_t mask = 0U) noexcept
    {
        if (text == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return frame(out, WsOpcode::text, text, std::strlen(text), fin, mask);
    }

    template <typename T, std::size_t Extent>
        requires WsBytes<T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> ping(
        std::span<std::uint8_t> out,
        const std::span<T, Extent> data = {},
        const std::uint32_t mask = 0U) noexcept
    {
        return frame(out, WsOpcode::ping, data.data(), data.size_bytes(), true, mask);
    }

    template <typename T, std::size_t Extent>
        requires WsBytes<T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> pong(
        std::span<std::uint8_t> out,
        const std::span<T, Extent> data = {},
        const std::uint32_t mask = 0U) noexcept
    {
        return frame(out, WsOpcode::pong, data.data(), data.size_bytes(), true, mask);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> close(
        std::span<std::uint8_t> out,
        const std::uint16_t code = 1000U,
        const std::span<const std::uint8_t> reason = {},
        const std::uint32_t mask = 0U) noexcept
    {
        if (reason.size() > 123U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::uint8_t payload[125]{};
        auto bytes = std::size_t{0U};
        if (code != 0U || !reason.empty()) {
            if (code == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            payload[bytes++] = static_cast<std::uint8_t>(code >> 8U);
            payload[bytes++] = static_cast<std::uint8_t>(code & 0xffU);
            if (!reason.empty()) {
                std::memcpy(payload + bytes, reason.data(), reason.size());
                bytes += reason.size();
            }
        }
        return frame(out, WsOpcode::close, payload, bytes, true, mask);
    }

    [[nodiscard]] static Result<WsFrame> parse(
        const std::span<const std::uint8_t> frame,
        const std::span<std::uint8_t> scratch = {}) noexcept
    {
        if (frame.size() < 2U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto byte0 = frame[0];
        const auto byte1 = frame[1];
        const auto fin = (byte0 & 0x80U) != 0U;
        const auto masked = (byte1 & 0x80U) != 0U;
        auto pos = std::size_t{2U};
        auto bytes = std::uint64_t{static_cast<std::uint64_t>(byte1 & 0x7fU)};

        if (bytes == 126U) {
            if (frame.size() < pos + 2U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            bytes = read_u16(frame, pos);
            pos += 2U;
        } else if (bytes == 127U) {
            if (frame.size() < pos + 8U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            bytes = read_u64(frame, pos);
            pos += 8U;
            if ((bytes >> 63U) != 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
        }

        std::uint32_t mask = 0U;
        if (masked) {
            if (frame.size() < pos + 4U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            mask = read_u32(frame, pos);
            pos += 4U;
        }

        if (bytes > static_cast<std::uint64_t>(frame.size() - pos)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto opcode = static_cast<WsOpcode>(byte0 & 0x0fU);
        if (!valid_opcode(opcode)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (control(opcode) && (!fin || bytes > 125U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::span<const std::uint8_t> payload{};
        if (masked) {
            if (scratch.size() < bytes) {
                return fail(ESP_ERR_NO_MEM);
            }
            for (std::size_t i = 0; i < bytes; ++i) {
                scratch[i] = static_cast<std::uint8_t>(frame[pos + i] ^ mask_byte(mask, i));
            }
            payload = scratch.first(static_cast<std::size_t>(bytes));
        } else {
            payload = frame.subspan(pos, static_cast<std::size_t>(bytes));
        }

        return WsFrame{
            .opcode = opcode,
            .payload = payload,
            .bytes = pos + static_cast<std::size_t>(bytes),
            .mask = mask,
            .fin = fin,
            .rsv1 = (byte0 & 0x40U) != 0U,
            .rsv2 = (byte0 & 0x20U) != 0U,
            .rsv3 = (byte0 & 0x10U) != 0U,
            .masked = masked,
        };
    }

    [[nodiscard]] static Result<WsClose> close_view(const WsFrame& frame) noexcept
    {
        if (frame.opcode != WsOpcode::close) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (frame.payload.empty()) {
            return WsClose{};
        }
        if (frame.payload.size() < 2U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return WsClose{
            .code = read_u16(frame.payload, 0U),
            .reason = frame.payload.subspan(2U),
        };
    }

private:
    struct Sha1 {
        std::uint32_t state[5]{
            0x67452301U,
            0xEFCDAB89U,
            0x98BADCFEU,
            0x10325476U,
            0xC3D2E1F0U,
        };
        std::uint8_t block[64]{};
        std::uint64_t bits{};
        std::size_t used{};

        void update(const std::uint8_t* data, std::size_t bytes) noexcept
        {
            bits += static_cast<std::uint64_t>(bytes) * 8U;

            while (bytes != 0U) {
                const auto room = sizeof(block) - used;
                const auto take = bytes < room ? bytes : room;
                std::memcpy(block + used, data, take);
                used += take;
                data += take;
                bytes -= take;

                if (used == sizeof(block)) {
                    compress(block);
                    used = 0U;
                }
            }
        }

        void final(std::uint8_t out[20]) noexcept
        {
            block[used++] = 0x80U;
            if (used > 56U) {
                while (used < sizeof(block)) {
                    block[used++] = 0U;
                }
                compress(block);
                used = 0U;
            }

            while (used < 56U) {
                block[used++] = 0U;
            }
            for (std::size_t i = 0; i < 8U; ++i) {
                block[56U + i] = static_cast<std::uint8_t>(bits >> ((7U - i) * 8U));
            }
            compress(block);

            for (std::size_t i = 0; i < 5U; ++i) {
                out[i * 4U + 0U] = static_cast<std::uint8_t>(state[i] >> 24U);
                out[i * 4U + 1U] = static_cast<std::uint8_t>((state[i] >> 16U) & 0xffU);
                out[i * 4U + 2U] = static_cast<std::uint8_t>((state[i] >> 8U) & 0xffU);
                out[i * 4U + 3U] = static_cast<std::uint8_t>(state[i] & 0xffU);
            }
        }

        static constexpr std::uint32_t rol(const std::uint32_t value, const unsigned bits) noexcept
        {
            return (value << bits) | (value >> (32U - bits));
        }

        void compress(const std::uint8_t input[64]) noexcept
        {
            std::uint32_t w[80]{};
            for (std::size_t i = 0; i < 16U; ++i) {
                const auto j = i * 4U;
                w[i] =
                    (static_cast<std::uint32_t>(input[j + 0U]) << 24U) |
                    (static_cast<std::uint32_t>(input[j + 1U]) << 16U) |
                    (static_cast<std::uint32_t>(input[j + 2U]) << 8U) |
                    static_cast<std::uint32_t>(input[j + 3U]);
            }
            for (std::size_t i = 16U; i < 80U; ++i) {
                w[i] = rol(w[i - 3U] ^ w[i - 8U] ^ w[i - 14U] ^ w[i - 16U], 1U);
            }

            auto a = state[0];
            auto b = state[1];
            auto c = state[2];
            auto d = state[3];
            auto e = state[4];

            for (std::size_t i = 0; i < 80U; ++i) {
                std::uint32_t f{};
                std::uint32_t k{};
                if (i < 20U) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999U;
                } else if (i < 40U) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1U;
                } else if (i < 60U) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDCU;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6U;
                }

                const auto temp = rol(a, 5U) + f + e + k + w[i];
                e = d;
                d = c;
                c = rol(b, 30U);
                b = a;
                a = temp;
            }

            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
        }
    };

    inline constexpr static char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    inline constexpr static char guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    [[nodiscard]] static constexpr bool control(const WsOpcode opcode) noexcept
    {
        return static_cast<std::uint8_t>(opcode) >= 0x8U;
    }

    [[nodiscard]] static constexpr bool valid_opcode(const WsOpcode opcode) noexcept
    {
        switch (opcode) {
            case WsOpcode::continuation:
            case WsOpcode::text:
            case WsOpcode::binary:
            case WsOpcode::close:
            case WsOpcode::ping:
            case WsOpcode::pong:
                return true;
        }
        return false;
    }

    [[nodiscard]] static constexpr std::size_t header_bytes(
        const std::size_t bytes,
        const bool masked) noexcept
    {
        return 2U + (bytes < 126U ? 0U : bytes <= 0xffffU ? 2U
                                                          : 8U) +
            (masked ? 4U : 0U);
    }

    static std::size_t write_length(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::size_t bytes,
        const bool masked) noexcept
    {
        const auto mask = static_cast<std::uint8_t>(masked ? 0x80U : 0U);
        if (bytes < 126U) {
            out[pos] = static_cast<std::uint8_t>(mask | bytes);
            return 1U;
        }
        if (bytes <= 0xffffU) {
            out[pos] = static_cast<std::uint8_t>(mask | 126U);
            out[pos + 1U] = static_cast<std::uint8_t>(bytes >> 8U);
            out[pos + 2U] = static_cast<std::uint8_t>(bytes & 0xffU);
            return 3U;
        }

        out[pos] = static_cast<std::uint8_t>(mask | 127U);
        for (std::size_t i = 0; i < 8U; ++i) {
            out[pos + 1U + i] = static_cast<std::uint8_t>(
                static_cast<std::uint64_t>(bytes) >> ((7U - i) * 8U));
        }
        return 9U;
    }

    static std::size_t write_u32(
        const std::span<std::uint8_t> out,
        const std::size_t pos,
        const std::uint32_t value) noexcept
    {
        out[pos + 0U] = static_cast<std::uint8_t>(value >> 24U);
        out[pos + 1U] = static_cast<std::uint8_t>((value >> 16U) & 0xffU);
        out[pos + 2U] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
        out[pos + 3U] = static_cast<std::uint8_t>(value & 0xffU);
        return 4U;
    }

    [[nodiscard]] static constexpr std::uint8_t mask_byte(
        const std::uint32_t mask,
        const std::size_t index) noexcept
    {
        return static_cast<std::uint8_t>(mask >> ((3U - (index & 3U)) * 8U));
    }

    [[nodiscard]] static Result<std::span<const char>> base64(
        std::span<char> out,
        const std::span<const std::uint8_t> data) noexcept
    {
        const auto need = ((data.size() + 2U) / 3U) * 4U;
        if (out.size() < need) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto pos = std::size_t{0U};
        auto i = std::size_t{0U};
        while (i + 3U <= data.size()) {
            const auto chunk =
                (static_cast<std::uint32_t>(data[i + 0U]) << 16U) |
                (static_cast<std::uint32_t>(data[i + 1U]) << 8U) |
                static_cast<std::uint32_t>(data[i + 2U]);
            out[pos++] = b64[(chunk >> 18U) & 0x3fU];
            out[pos++] = b64[(chunk >> 12U) & 0x3fU];
            out[pos++] = b64[(chunk >> 6U) & 0x3fU];
            out[pos++] = b64[chunk & 0x3fU];
            i += 3U;
        }

        const auto tail = data.size() - i;
        if (tail == 1U) {
            const auto chunk = static_cast<std::uint32_t>(data[i]) << 16U;
            out[pos++] = b64[(chunk >> 18U) & 0x3fU];
            out[pos++] = b64[(chunk >> 12U) & 0x3fU];
            out[pos++] = '=';
            out[pos++] = '=';
        } else if (tail == 2U) {
            const auto chunk =
                (static_cast<std::uint32_t>(data[i + 0U]) << 16U) |
                (static_cast<std::uint32_t>(data[i + 1U]) << 8U);
            out[pos++] = b64[(chunk >> 18U) & 0x3fU];
            out[pos++] = b64[(chunk >> 12U) & 0x3fU];
            out[pos++] = b64[(chunk >> 6U) & 0x3fU];
            out[pos++] = '=';
        }

        return std::span<const char>{out.data(), pos};
    }

    [[nodiscard]] static std::uint16_t read_u16(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(in[pos]) << 8U) | in[pos + 1U]);
    }

    [[nodiscard]] static std::uint32_t read_u32(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        return (static_cast<std::uint32_t>(in[pos + 0U]) << 24U) |
            (static_cast<std::uint32_t>(in[pos + 1U]) << 16U) |
            (static_cast<std::uint32_t>(in[pos + 2U]) << 8U) |
            static_cast<std::uint32_t>(in[pos + 3U]);
    }

    [[nodiscard]] static std::uint64_t read_u64(
        const std::span<const std::uint8_t> in,
        const std::size_t pos) noexcept
    {
        auto value = std::uint64_t{0U};
        for (std::size_t i = 0; i < 8U; ++i) {
            value = (value << 8U) | in[pos + i];
        }
        return value;
    }
};

template <typename Codec>
concept WsAdapter = requires(
    Codec& codec,
    std::span<char> text_out,
    std::span<std::uint8_t> out,
    std::span<const std::uint8_t> bytes,
    const char* key,
    const char* text,
    const WsFrame& frame) {
    { codec.key(text_out, bytes) } -> std::same_as<Result<std::span<const char>>>;
    { codec.accept(text_out, key) } -> std::same_as<Result<std::span<const char>>>;
    { codec.frame(out, WsOpcode::binary, bytes.data(), bytes.size(), true, std::uint32_t{}, false, false, false) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.text(out, text, true, std::uint32_t{}) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.close(out, std::uint16_t{}, bytes, std::uint32_t{}) } -> std::same_as<Result<std::span<const std::uint8_t>>>;
    { codec.parse(bytes, out) } -> std::same_as<Result<WsFrame>>;
    { codec.close_view(frame) } -> std::same_as<Result<WsClose>>;
};

struct AnyWs {
    using KeyFn = Result<std::span<const char>> (*)(void*, std::span<char>, std::span<const std::uint8_t>) noexcept;
    using AcceptFn = Result<std::span<const char>> (*)(void*, std::span<char>, const char*) noexcept;
    using FrameFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        WsOpcode,
        const void*,
        std::size_t,
        bool,
        std::uint32_t,
        bool,
        bool,
        bool) noexcept;
    using TextFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        const char*,
        bool,
        std::uint32_t) noexcept;
    using CloseFn = Result<std::span<const std::uint8_t>> (*)(
        void*,
        std::span<std::uint8_t>,
        std::uint16_t,
        std::span<const std::uint8_t>,
        std::uint32_t) noexcept;
    using ParseFn = Result<WsFrame> (*)(void*, std::span<const std::uint8_t>, std::span<std::uint8_t>) noexcept;
    using CloseViewFn = Result<WsClose> (*)(void*, const WsFrame&) noexcept;

    void* ctx{};
    KeyFn key_fn{};
    AcceptFn accept_fn{};
    FrameFn frame_fn{};
    TextFn text_fn{};
    CloseFn close_fn{};
    ParseFn parse_fn{};
    CloseViewFn close_view_fn{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return key_fn != nullptr &&
            accept_fn != nullptr &&
            frame_fn != nullptr &&
            text_fn != nullptr &&
            close_fn != nullptr &&
            parse_fn != nullptr &&
            close_view_fn != nullptr;
    }

    [[nodiscard]] static constexpr AnyWs arc() noexcept
    {
        return {
            .ctx = nullptr,
            .key_fn = &arc_key,
            .accept_fn = &arc_accept,
            .frame_fn = &arc_frame,
            .text_fn = &arc_text,
            .close_fn = &arc_close,
            .parse_fn = &arc_parse,
            .close_view_fn = &arc_close_view,
        };
    }

    template <WsAdapter Codec>
    [[nodiscard]] static constexpr AnyWs bind(Codec& codec) noexcept
    {
        return {
            .ctx = &codec,
            .key_fn = &key_object<Codec>,
            .accept_fn = &accept_object<Codec>,
            .frame_fn = &frame_object<Codec>,
            .text_fn = &text_object<Codec>,
            .close_fn = &close_object<Codec>,
            .parse_fn = &parse_object<Codec>,
            .close_view_fn = &close_view_object<Codec>,
        };
    }

    [[nodiscard]] Result<std::span<const char>> key(
        std::span<char> out,
        const std::span<const std::uint8_t> nonce) noexcept
    {
        return key_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : key_fn(ctx, out, nonce);
    }

    [[nodiscard]] Result<std::span<const char>> accept(
        std::span<char> out,
        const char* const key) noexcept
    {
        return accept_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : accept_fn(ctx, out, key);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> frame(
        std::span<std::uint8_t> out,
        const WsOpcode opcode,
        const void* const data,
        const std::size_t bytes,
        const bool fin = true,
        const std::uint32_t mask = 0U,
        const bool rsv1 = false,
        const bool rsv2 = false,
        const bool rsv3 = false) noexcept
    {
        return frame_fn == nullptr ? fail(ESP_ERR_INVALID_STATE)
                                   : frame_fn(ctx, out, opcode, data, bytes, fin, mask, rsv1, rsv2, rsv3);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> text(
        std::span<std::uint8_t> out,
        const char* const value,
        const bool fin = true,
        const std::uint32_t mask = 0U) noexcept
    {
        return text_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : text_fn(ctx, out, value, fin, mask);
    }

    [[nodiscard]] Result<std::span<const std::uint8_t>> close(
        std::span<std::uint8_t> out,
        const std::uint16_t code = 1000U,
        const std::span<const std::uint8_t> reason = {},
        const std::uint32_t mask = 0U) noexcept
    {
        return close_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : close_fn(ctx, out, code, reason, mask);
    }

    [[nodiscard]] Result<WsFrame> parse(
        const std::span<const std::uint8_t> in,
        const std::span<std::uint8_t> scratch = {}) noexcept
    {
        return parse_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : parse_fn(ctx, in, scratch);
    }

    [[nodiscard]] Result<WsClose> close_view(const WsFrame& frame) noexcept
    {
        return close_view_fn == nullptr ? fail(ESP_ERR_INVALID_STATE) : close_view_fn(ctx, frame);
    }

private:
    [[nodiscard]] static Result<std::span<const char>> arc_key(
        void*,
        const std::span<char> out,
        const std::span<const std::uint8_t> nonce) noexcept
    {
        return Ws::key(out, nonce);
    }

    [[nodiscard]] static Result<std::span<const char>> arc_accept(
        void*,
        const std::span<char> out,
        const char* const key) noexcept
    {
        return Ws::accept(out, key);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_frame(
        void*,
        const std::span<std::uint8_t> out,
        const WsOpcode opcode,
        const void* const data,
        const std::size_t bytes,
        const bool fin,
        const std::uint32_t mask,
        const bool rsv1,
        const bool rsv2,
        const bool rsv3) noexcept
    {
        return Ws::frame(out, opcode, data, bytes, fin, mask, rsv1, rsv2, rsv3);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_text(
        void*,
        const std::span<std::uint8_t> out,
        const char* const value,
        const bool fin,
        const std::uint32_t mask) noexcept
    {
        return Ws::text(out, value, fin, mask);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> arc_close(
        void*,
        const std::span<std::uint8_t> out,
        const std::uint16_t code,
        const std::span<const std::uint8_t> reason,
        const std::uint32_t mask) noexcept
    {
        return Ws::close(out, code, reason, mask);
    }

    [[nodiscard]] static Result<WsFrame> arc_parse(
        void*,
        const std::span<const std::uint8_t> in,
        const std::span<std::uint8_t> scratch) noexcept
    {
        return Ws::parse(in, scratch);
    }

    [[nodiscard]] static Result<WsClose> arc_close_view(
        void*,
        const WsFrame& frame) noexcept
    {
        return Ws::close_view(frame);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<std::span<const char>> key_object(
        void* const ctx,
        const std::span<char> out,
        const std::span<const std::uint8_t> nonce) noexcept
    {
        return static_cast<Codec*>(ctx)->key(out, nonce);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<std::span<const char>> accept_object(
        void* const ctx,
        const std::span<char> out,
        const char* const key) noexcept
    {
        return static_cast<Codec*>(ctx)->accept(out, key);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> frame_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const WsOpcode opcode,
        const void* const data,
        const std::size_t bytes,
        const bool fin,
        const std::uint32_t mask,
        const bool rsv1,
        const bool rsv2,
        const bool rsv3) noexcept
    {
        return static_cast<Codec*>(ctx)->frame(out, opcode, data, bytes, fin, mask, rsv1, rsv2, rsv3);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> text_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const char* const value,
        const bool fin,
        const std::uint32_t mask) noexcept
    {
        return static_cast<Codec*>(ctx)->text(out, value, fin, mask);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> close_object(
        void* const ctx,
        const std::span<std::uint8_t> out,
        const std::uint16_t code,
        const std::span<const std::uint8_t> reason,
        const std::uint32_t mask) noexcept
    {
        return static_cast<Codec*>(ctx)->close(out, code, reason, mask);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<WsFrame> parse_object(
        void* const ctx,
        const std::span<const std::uint8_t> in,
        const std::span<std::uint8_t> scratch) noexcept
    {
        return static_cast<Codec*>(ctx)->parse(in, scratch);
    }

    template <WsAdapter Codec>
    [[nodiscard]] static Result<WsClose> close_view_object(
        void* const ctx,
        const WsFrame& frame) noexcept
    {
        return static_cast<Codec*>(ctx)->close_view(frame);
    }
};

}  // namespace arc::net
