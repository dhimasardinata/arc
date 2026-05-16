#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc::net {

struct UriView {
    std::span<const char> scheme{};
    std::span<const char> authority{};
    std::span<const char> userinfo{};
    std::span<const char> host{};
    std::span<const char> port{};
    std::span<const char> path{};
    std::span<const char> query{};
    std::span<const char> fragment{};
    bool has_query{};

    [[nodiscard]] constexpr bool absolute() const noexcept
    {
        return !scheme.empty();
    }

    [[nodiscard]] constexpr bool has_authority() const noexcept
    {
        return !authority.empty();
    }
};

struct UriQueryParam {
    std::span<const char> key{};
    std::span<const char> value{};
    bool has_value{};
};

struct UriEndpoint {
    std::span<const char> host{};
    std::uint16_t port{};
};

struct Uri {
    [[nodiscard]] static Result<UriView> parse(const char* const text) noexcept
    {
        if (text == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return parse(std::span<const char>{text, std::strlen(text)});
    }

    [[nodiscard]] static Result<UriView> parse(const std::span<const char> in) noexcept
    {
        if (!valid_text(in)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        UriView out{};
        auto pos = std::size_t{0U};

        const auto first = find_any(in, 0U, ":/?#");
        if (first != npos && in[first] == ':') {
            out.scheme = in.first(first);
            if (!valid_scheme(out.scheme)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            pos = first + 1U;
        }

        if (starts(in, pos, "//")) {
            const auto start = pos + 2U;
            auto end = find_any(in, start, "/?#");
            if (end == npos) {
                end = in.size();
            }
            out.authority = in.subspan(start, end - start);
            if (!split_authority(out)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            pos = end;
        }

        const auto fragment = find_char(in, '#', pos, in.size());
        const auto limit = fragment == npos ? in.size() : fragment;
        const auto query = find_char(in, '?', pos, limit);
        const auto path_end = query == npos ? limit : query;
        out.path = in.subspan(pos, path_end - pos);
        if (query != npos) {
            out.query = in.subspan(query + 1U, limit - query - 1U);
            out.has_query = true;
        }
        if (fragment != npos) {
            out.fragment = in.subspan(fragment + 1U);
        }
        if (!valid_path(out.path) || (out.has_query && !valid_query(out.query)) ||
            !valid_fragment(out.fragment)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return out;
    }

    [[nodiscard]] static Result<UriQueryParam> next(
        const std::span<const char> query,
        std::size_t& offset) noexcept
    {
        if (offset > query.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        while (offset < query.size() && query[offset] == '&') {
            ++offset;
        }
        if (offset >= query.size()) {
            return fail(ESP_ERR_NOT_FOUND);
        }

        const auto start = offset;
        auto end = find_char(query, '&', start, query.size());
        if (end == npos) {
            end = query.size();
            offset = end;
        } else {
            offset = end + 1U;
        }

        const auto part = query.subspan(start, end - start);
        const auto equal = find_char(part, '=', 0U, part.size());
        if (equal == npos) {
            return UriQueryParam{.key = part};
        }
        return UriQueryParam{
            .key = part.first(equal),
            .value = part.subspan(equal + 1U),
            .has_value = true,
        };
    }

    [[nodiscard]] static bool scheme_is(const UriView& uri, const char* const scheme) noexcept
    {
        if (scheme == nullptr) {
            return false;
        }

        auto len = std::size_t{0U};
        while (scheme[len] != '\0') {
            ++len;
        }
        if (uri.scheme.size() != len) {
            return false;
        }
        for (std::size_t i = 0U; i < len; ++i) {
            if (lower(uri.scheme[i]) != lower(scheme[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static Result<std::uint16_t> port(
        const UriView& uri,
        const std::uint16_t fallback = 0U) noexcept
    {
        if (uri.port.empty()) {
            if (fallback == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            return fallback;
        }

        std::uint32_t value{};
        if (!parse_port(uri.port, value)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return static_cast<std::uint16_t>(value);
    }

    [[nodiscard]] static Result<UriEndpoint> endpoint(
        const UriView& uri,
        const std::uint16_t fallback_port = 0U) noexcept
    {
        if (!uri.has_authority() || uri.host.empty() || !valid_endpoint_host(uri.host)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto value = port(uri, fallback_port);
        if (!value || *value == 0U) {
            return fail(value ? ESP_ERR_INVALID_ARG : value.error());
        }
        return UriEndpoint{.host = uri.host, .port = *value};
    }

    [[nodiscard]] static Result<std::span<char>> copy_host(
        const std::span<char> out,
        const UriView& uri) noexcept
    {
        if (uri.host.empty() || !valid_endpoint_host(uri.host)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        auto used = std::size_t{0U};
        for (std::size_t i = 0U; i < uri.host.size(); ++i) {
            if (used + 1U >= out.size()) {
                return fail(ESP_ERR_NO_MEM);
            }
            if (uri.host[i] == '%') {
                if (i + 2U >= uri.host.size()) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                const auto hi = hex(uri.host[i + 1U]);
                const auto lo = hex(uri.host[i + 2U]);
                if (hi < 0 || lo < 0) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                const auto value = static_cast<char>((static_cast<unsigned>(hi) << 4U) | static_cast<unsigned>(lo));
                if (!decoded_host_char(uri.host, i, value)) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                out[used++] = value;
                i += 2U;
            } else if (!raw_host_char(uri.host[i])) {
                return fail(ESP_ERR_INVALID_ARG);
            } else {
                out[used++] = uri.host[i];
            }
        }
        out[used] = '\0';
        return out.first(used);
    }

    [[nodiscard]] static Result<std::span<char>> path_query(
        const std::span<char> out,
        const UriView& uri) noexcept
    {
        if (!valid_path(uri.path) || (uri.has_query && !valid_query(uri.query))) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        auto used = std::size_t{0U};
        if (uri.path.empty()) {
            if (!put(out, used, '/')) {
                return fail(ESP_ERR_NO_MEM);
            }
        } else if (!append(out, used, uri.path)) {
            return fail(ESP_ERR_NO_MEM);
        }

        if (uri.has_query) {
            if (!put(out, used, '?') || !append(out, used, uri.query)) {
                return fail(ESP_ERR_NO_MEM);
            }
        }
        return out.first(used);
    }

    [[nodiscard]] static Result<std::span<char>> decode(
        const std::span<char> out,
        const char* const in,
        const bool plus_space = false) noexcept
    {
        if (in == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return decode(out, std::span<const char>{in, std::strlen(in)}, plus_space);
    }

    [[nodiscard]] static Result<std::span<char>> decode(
        const std::span<char> out,
        const std::span<const char> in,
        const bool plus_space = false) noexcept
    {
        auto used = std::size_t{0U};
        for (std::size_t i = 0U; i < in.size(); ++i) {
            if (used >= out.size()) {
                return fail(ESP_ERR_NO_MEM);
            }
            if (in[i] == '%') {
                if (i + 2U >= in.size()) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                const auto hi = hex(in[i + 1U]);
                const auto lo = hex(in[i + 2U]);
                if (hi < 0 || lo < 0) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                out[used++] = static_cast<char>((static_cast<unsigned>(hi) << 4U) | static_cast<unsigned>(lo));
                i += 2U;
            } else {
                out[used++] = plus_space && in[i] == '+' ? ' ' : in[i];
            }
        }
        return out.first(used);
    }

    [[nodiscard]] static Result<std::span<char>> encode(
        const std::span<char> out,
        const char* const in,
        const bool space_plus = false) noexcept
    {
        if (in == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return encode(out, std::span<const char>{in, std::strlen(in)}, space_plus);
    }

    [[nodiscard]] static Result<std::span<char>> encode(
        const std::span<char> out,
        const std::span<const char> in,
        const bool space_plus = false) noexcept
    {
        auto used = std::size_t{0U};
        for (const char ch : in) {
            const auto byte = static_cast<std::uint8_t>(ch);
            if (unreserved(byte)) {
                if (!put(out, used, ch)) {
                    return fail(ESP_ERR_NO_MEM);
                }
            } else if (space_plus && ch == ' ') {
                if (!put(out, used, '+')) {
                    return fail(ESP_ERR_NO_MEM);
                }
            } else {
                if (used + 3U > out.size()) {
                    return fail(ESP_ERR_NO_MEM);
                }
                out[used++] = '%';
                out[used++] = hex_char(byte >> 4U);
                out[used++] = hex_char(byte & 0x0fU);
            }
        }
        return out.first(used);
    }

private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    [[nodiscard]] static bool split_authority(UriView& out) noexcept
    {
        if (out.authority.empty()) {
            return false;
        }

        auto hostport = out.authority;
        const auto at = find_last(out.authority, '@');
        if (at != npos) {
            out.userinfo = out.authority.first(at);
            hostport = out.authority.subspan(at + 1U);
            if (out.userinfo.empty() || hostport.empty() || !valid_userinfo(out.userinfo)) {
                return false;
            }
        }

        if (hostport.empty()) {
            return false;
        }
        if (hostport[0] == '[') {
            const auto close = find_char(hostport, ']', 1U, hostport.size());
            if (close == npos || close == 1U) {
                return false;
            }
            out.host = hostport.subspan(1U, close - 1U);
            if (!valid_lit(out.host)) {
                return false;
            }
            if (close + 1U == hostport.size()) {
                return true;
            }
            if (hostport[close + 1U] != ':') {
                return false;
            }
            out.port = hostport.subspan(close + 2U);
            return valid_port(out.port);
        }

        const auto colon = find_char(hostport, ':', 0U, hostport.size());
        if (colon == npos) {
            out.host = hostport;
            return valid_host(out.host);
        }
        if (find_char(hostport, ':', colon + 1U, hostport.size()) != npos) {
            return false;
        }
        out.host = hostport.first(colon);
        out.port = hostport.subspan(colon + 1U);
        return valid_host(out.host) && valid_port(out.port);
    }

    [[nodiscard]] static bool valid_userinfo(const std::span<const char> text) noexcept
    {
        for (std::size_t i = 0U; i < text.size(); ++i) {
            const auto ch = text[i];
            if (ch == '%') {
                if (i + 2U >= text.size()) {
                    return false;
                }
                const auto hi = hex(text[i + 1U]);
                const auto lo = hex(text[i + 2U]);
                if (hi < 0 || lo < 0) {
                    return false;
                }
                const auto value = static_cast<char>((static_cast<unsigned>(hi) << 4U) | static_cast<unsigned>(lo));
                if (!userinfo_char(value)) {
                    return false;
                }
                i += 2U;
                continue;
            }
            if (!userinfo_char(ch)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_host(const std::span<const char> text) noexcept
    {
        if (text.empty()) {
            return false;
        }
        for (std::size_t i = 0U; i < text.size(); ++i) {
            const auto ch = text[i];
            if (unreserved(static_cast<std::uint8_t>(ch)) || subdelim(ch)) {
                continue;
            }
            if (ch == '%' && i + 2U < text.size() && hex(text[i + 1U]) >= 0 && hex(text[i + 2U]) >= 0) {
                i += 2U;
                continue;
            }
            return false;
        }
        return true;
    }

    [[nodiscard]] static bool valid_endpoint_host(const std::span<const char> text) noexcept
    {
        if (text.empty() || (contains(text, ':') && !valid_lit(text))) {
            return false;
        }
        for (std::size_t i = 0U; i < text.size(); ++i) {
            if (text[i] == '%') {
                if (i + 2U >= text.size()) {
                    return false;
                }
                const auto hi = hex(text[i + 1U]);
                const auto lo = hex(text[i + 2U]);
                if (hi < 0 || lo < 0) {
                    return false;
                }
                const auto value = static_cast<char>((static_cast<unsigned>(hi) << 4U) | static_cast<unsigned>(lo));
                if (!decoded_host_char(text, i, value)) {
                    return false;
                }
                i += 2U;
            } else if (!raw_host_char(text[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool unbracketed(const std::span<const char> text) noexcept
    {
        if (text.empty()) {
            return false;
        }
        for (const auto ch : text) {
            if (ch == '[' || ch == ']') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_lit(const std::span<const char> text) noexcept
    {
        if (!unbracketed(text)) {
            return false;
        }
        if (valid_v6(text)) {
            return true;
        }
        return valid_future(text);
    }

    [[nodiscard]] static bool valid_v6(const std::span<const char> text) noexcept
    {
        for (std::size_t i = 0U; i < text.size(); ++i) {
            if (text[i] == '%') {
                if (i + 2U >= text.size() || text[i + 1U] != '2' || text[i + 2U] != '5') {
                    return false;
                }
                return valid_v6_addr(text.first(i)) && valid_zone(text.subspan(i + 3U));
            }
        }
        return valid_v6_addr(text);
    }

    [[nodiscard]] static bool valid_v6_addr(const std::span<const char> text) noexcept
    {
        if (text.empty()) {
            return false;
        }

        auto pos = std::size_t{0U};
        auto groups = std::uint8_t{0U};
        auto compressed = false;

        if (starts(text, 0U, "::")) {
            compressed = true;
            pos = 2U;
            if (pos == text.size()) {
                return true;
            }
        } else if (text[0] == ':') {
            return false;
        }

        while (pos < text.size()) {
            if (groups >= 8U) {
                return false;
            }

            const auto start = pos;
            while (pos < text.size() && text[pos] != ':') {
                ++pos;
            }

            const auto part = text.subspan(start, pos - start);
            if (part.empty()) {
                return false;
            }
            if (contains(part, '.')) {
                if (!valid_v4_tail(text.subspan(start))) {
                    return false;
                }
                groups = static_cast<std::uint8_t>(groups + 2U);
                return groups <= 8U && (compressed ? groups < 8U : groups == 8U);
            }
            if (!valid_h16(part)) {
                return false;
            }
            ++groups;

            if (pos == text.size()) {
                break;
            }
            if (pos + 1U < text.size() && text[pos + 1U] == ':') {
                if (compressed) {
                    return false;
                }
                compressed = true;
                pos += 2U;
                if (pos == text.size()) {
                    break;
                }
            } else {
                ++pos;
                if (pos == text.size()) {
                    return false;
                }
            }
        }

        return compressed ? groups < 8U : groups == 8U;
    }

    [[nodiscard]] static bool valid_h16(const std::span<const char> text) noexcept
    {
        if (text.empty() || text.size() > 4U) {
            return false;
        }
        for (const auto ch : text) {
            if (hex(ch) < 0) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_v4_tail(const std::span<const char> text) noexcept
    {
        auto pos = std::size_t{0U};
        auto octets = std::uint8_t{0U};
        while (pos < text.size()) {
            const auto start = pos;
            auto value = std::uint16_t{0U};
            while (pos < text.size() && text[pos] != '.') {
                if (!digit(text[pos])) {
                    return false;
                }
                value = static_cast<std::uint16_t>((value * 10U) + static_cast<std::uint16_t>(text[pos] - '0'));
                if (value > 255U) {
                    return false;
                }
                ++pos;
            }
            if (pos == start || (pos - start > 1U && text[start] == '0')) {
                return false;
            }
            ++octets;
            if (pos == text.size()) {
                break;
            }
            ++pos;
        }
        return octets == 4U;
    }

    [[nodiscard]] static bool valid_zone(const std::span<const char> text) noexcept
    {
        if (text.empty()) {
            return false;
        }
        for (std::size_t i = 0U; i < text.size(); ++i) {
            if (unreserved(static_cast<std::uint8_t>(text[i])) || subdelim(text[i]) || text[i] == ':') {
                continue;
            }
            if (text[i] == '%' && i + 2U < text.size() && hex(text[i + 1U]) >= 0 && hex(text[i + 2U]) >= 0) {
                i += 2U;
                continue;
            }
            return false;
        }
        return true;
    }

    [[nodiscard]] static bool valid_future(const std::span<const char> text) noexcept
    {
        if (text.size() < 4U || lower(text[0]) != 'v') {
            return false;
        }
        auto pos = std::size_t{1U};
        const auto hex_start = pos;
        while (pos < text.size() && hex(text[pos]) >= 0) {
            ++pos;
        }
        if (pos == hex_start || pos >= text.size() || text[pos] != '.') {
            return false;
        }
        ++pos;
        if (pos >= text.size()) {
            return false;
        }
        for (; pos < text.size(); ++pos) {
            if (!unreserved(static_cast<std::uint8_t>(text[pos])) && !subdelim(text[pos]) && text[pos] != ':') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_scheme(const std::span<const char> text) noexcept
    {
        if (text.empty() || !alpha(text[0])) {
            return false;
        }
        for (const auto ch : text.subspan(1U)) {
            if (!alpha(ch) && !digit(ch) && ch != '+' && ch != '-' && ch != '.') {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_text(const std::span<const char> text) noexcept
    {
        for (std::size_t i = 0U; i < text.size(); ++i) {
            const auto ch = text[i];
            const auto byte = static_cast<std::uint8_t>(ch);
            if (byte <= 0x20U || byte == 0x7fU) {
                return false;
            }
            if (ch == '%') {
                if (i + 2U >= text.size() || hex(text[i + 1U]) < 0 || hex(text[i + 2U]) < 0) {
                    return false;
                }
                i += 2U;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_path(const std::span<const char> text) noexcept
    {
        return valid_component(text, false);
    }

    [[nodiscard]] static bool valid_query(const std::span<const char> text) noexcept
    {
        return valid_component(text, true);
    }

    [[nodiscard]] static bool valid_fragment(const std::span<const char> text) noexcept
    {
        return valid_component(text, true);
    }

    [[nodiscard]] static bool valid_component(
        const std::span<const char> text,
        const bool allow_question) noexcept
    {
        for (std::size_t i = 0U; i < text.size(); ++i) {
            if (text[i] == '%') {
                if (i + 2U >= text.size() || hex(text[i + 1U]) < 0 || hex(text[i + 2U]) < 0) {
                    return false;
                }
                i += 2U;
                continue;
            }
            if (!component_char(text[i]) && !(allow_question && text[i] == '?')) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool valid_port(const std::span<const char> text) noexcept
    {
        std::uint32_t value{};
        return parse_port(text, value);
    }

    [[nodiscard]] static bool parse_port(const std::span<const char> text, std::uint32_t& value) noexcept
    {
        if (text.empty()) {
            return false;
        }
        value = 0U;
        for (const auto ch : text) {
            if (!digit(ch)) {
                return false;
            }
            const auto digit_value = static_cast<std::uint32_t>(ch - '0');
            if (value > (65535U - digit_value) / 10U) {
                return false;
            }
            value = (value * 10U) + digit_value;
        }
        return true;
    }

    [[nodiscard]] static bool starts(const std::span<const char> text, const std::size_t pos, const char* prefix) noexcept
    {
        const auto len = std::strlen(prefix);
        if (pos + len > text.size()) {
            return false;
        }
        for (std::size_t i = 0U; i < len; ++i) {
            if (text[pos + i] != prefix[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool contains(const std::span<const char> text, const char needle) noexcept
    {
        for (const auto ch : text) {
            if (ch == needle) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] static bool raw_host_char(const char ch) noexcept
    {
        const auto byte = static_cast<std::uint8_t>(ch);
        return unreserved(byte) || subdelim(ch) || ch == ':';
    }

    [[nodiscard]] static bool userinfo_char(const char ch) noexcept
    {
        const auto byte = static_cast<std::uint8_t>(ch);
        if (byte <= 0x20U || byte == 0x7fU) {
            return false;
        }
        return unreserved(byte) || subdelim(ch) || ch == ':';
    }

    [[nodiscard]] static bool component_char(const char ch) noexcept
    {
        const auto byte = static_cast<std::uint8_t>(ch);
        if (byte <= 0x20U || byte == 0x7fU) {
            return false;
        }
        return unreserved(byte) || subdelim(ch) || ch == ':' || ch == '@' || ch == '/';
    }

    [[nodiscard]] static bool decoded_host_char(
        const std::span<const char> host,
        const std::size_t escape,
        const char ch) noexcept
    {
        const auto byte = static_cast<std::uint8_t>(ch);
        if (byte <= 0x20U || byte == 0x7fU) {
            return false;
        }
        if (unreserved(byte) || subdelim(ch)) {
            return true;
        }
        if (ch == ':' && contains(host, ':') && valid_lit(host)) {
            return true;
        }
        return ch == '%' && starts(host, escape, "%25") && contains(host, ':');
    }

    [[nodiscard]] static std::size_t find_any(
        const std::span<const char> text,
        const std::size_t start,
        const char* const chars) noexcept
    {
        for (std::size_t i = start; i < text.size(); ++i) {
            for (std::size_t j = 0U; chars[j] != '\0'; ++j) {
                if (text[i] == chars[j]) {
                    return i;
                }
            }
        }
        return npos;
    }

    [[nodiscard]] static std::size_t find_char(
        const std::span<const char> text,
        const char ch,
        const std::size_t start,
        const std::size_t limit) noexcept
    {
        for (std::size_t i = start; i < limit; ++i) {
            if (text[i] == ch) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] static std::size_t find_last(const std::span<const char> text, const char ch) noexcept
    {
        for (std::size_t i = text.size(); i > 0U; --i) {
            if (text[i - 1U] == ch) {
                return i - 1U;
            }
        }
        return npos;
    }

    [[nodiscard]] static bool put(const std::span<char> out, std::size_t& used, const char ch) noexcept
    {
        if (used >= out.size()) {
            return false;
        }
        out[used++] = ch;
        return true;
    }

    [[nodiscard]] static bool append(
        const std::span<char> out,
        std::size_t& used,
        const std::span<const char> text) noexcept
    {
        if (used > out.size() || text.size() > out.size() - used) {
            return false;
        }
        for (const auto ch : text) {
            out[used++] = ch;
        }
        return true;
    }

    [[nodiscard]] static bool alpha(const char ch) noexcept
    {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    }

    [[nodiscard]] static bool digit(const char ch) noexcept
    {
        return ch >= '0' && ch <= '9';
    }

    [[nodiscard]] static int hex(const char ch) noexcept
    {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'A' && ch <= 'F') {
            return ch - 'A' + 10;
        }
        if (ch >= 'a' && ch <= 'f') {
            return ch - 'a' + 10;
        }
        return -1;
    }

    [[nodiscard]] static char hex_char(const std::uint8_t value) noexcept
    {
        constexpr char digits[] = "0123456789ABCDEF";
        return digits[value & 0x0fU];
    }

    [[nodiscard]] static char lower(const char ch) noexcept
    {
        return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
    }

    [[nodiscard]] static bool unreserved(const std::uint8_t ch) noexcept
    {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '-' || ch == '.' || ch == '_' || ch == '~';
    }

    [[nodiscard]] static bool subdelim(const char ch) noexcept
    {
        return ch == '!' || ch == '$' || ch == '&' || ch == '\'' || ch == '(' || ch == ')' || ch == '*' ||
            ch == '+' || ch == ',' || ch == ';' || ch == '=';
    }
};

}  // namespace arc::net
