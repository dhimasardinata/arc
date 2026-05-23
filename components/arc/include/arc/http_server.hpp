#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "esp_err.h"

#include "arc/result.hpp"
#include "arc/text.hpp"
#include "arc/uri.hpp"

namespace arc::net {

enum class HttpMethod : std::uint8_t {
    unknown,
    get,
    head,
    post,
    put,
    patch,
    del,
    options,
};

struct HttpHeaderView {
    std::span<const char> name{};
    std::span<const char> value{};
};

struct HttpRequestView {
    HttpMethod method{};
    std::span<const char> target{};
    std::span<const char> version{};
    std::span<HttpHeaderView> headers{};
    std::span<const char> body{};
};

enum class JsonKind : std::uint8_t {
    null,
    boolean,
    u64,
    i64,
    str,
    raw,
};

struct Json {
    JsonKind kind{JsonKind::null};
    std::string_view text{};
    std::uint64_t u{};
    std::int64_t i{};
    bool flag{};

    [[nodiscard]] static constexpr Json nil() noexcept
    {
        return {};
    }

    [[nodiscard]] static constexpr Json boolean(const bool value) noexcept
    {
        return {.kind = JsonKind::boolean, .flag = value};
    }

    [[nodiscard]] static constexpr Json u64(const std::uint64_t value) noexcept
    {
        return {.kind = JsonKind::u64, .u = value};
    }

    [[nodiscard]] static constexpr Json i64(const std::int64_t value) noexcept
    {
        return {.kind = JsonKind::i64, .i = value};
    }

    [[nodiscard]] static constexpr Json str(const std::string_view value) noexcept
    {
        return {.kind = JsonKind::str, .text = value};
    }

    [[nodiscard]] static constexpr Json raw(const std::string_view value) noexcept
    {
        return {.kind = JsonKind::raw, .text = value};
    }
};

struct JsonField {
    std::string_view name{};
    Json value{};
};

template <std::size_t N>
struct HttpStaticText {
    char value[N]{};

    consteval HttpStaticText(const char (&text)[N]) noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            value[i] = text[i];
        }
    }

    [[nodiscard]] static consteval std::size_t size() noexcept
    {
        return N - 1U;
    }
};

template <HttpMethod Method, HttpStaticText Path>
struct HttpRoute {
    static constexpr HttpMethod method = Method;
    static constexpr auto path = Path;

    [[nodiscard]] static consteval std::uint32_t id() noexcept
    {
        std::uint32_t out = 2'166'136'261U;
        for (std::size_t i = 0; i < Path.size(); ++i) {
            out ^= static_cast<std::uint8_t>(Path.value[i]);
            out *= 16'777'619U;
        }
        out ^= static_cast<std::uint8_t>(Method);
        out *= 16'777'619U;
        return out == 0U ? 1U : out;
    }

    [[nodiscard]] static constexpr bool matches(const HttpRequestView& req) noexcept
    {
        return req.method == Method && equal(route_path(req.target), std::span<const char>{Path.value, Path.size()});
    }

private:
    [[nodiscard]] static constexpr std::span<const char> route_path(const std::span<const char> target) noexcept
    {
        for (std::size_t i = 0U; i < target.size(); ++i) {
            if (target[i] == '?') {
                return target.first(i);
            }
        }
        return target;
    }

    [[nodiscard]] static constexpr bool equal(
        const std::span<const char> lhs,
        const std::span<const char> rhs) noexcept
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }
};

template <typename... Routes>
struct HttpRouter {
    template <typename Fn>
    [[nodiscard]] static bool dispatch(const HttpRequestView& req, Fn&& fn) noexcept((noexcept(fn(Routes{})) && ...))
    {
        return ((Routes::matches(req) ? (fn(Routes{}), true) : false) || ...);
    }
};

struct HttpServer {
    [[nodiscard]] static Result<HttpRequestView> parse(
        std::span<const char> in,
        std::span<HttpHeaderView> headers) noexcept
    {
        if (!valid_span(in) || !valid_span(headers)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto head_end = headers_end(in);
        if (head_end == npos) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto line_end = find_crlf(in, 0U, head_end);
        if (line_end == npos) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto request = in.first(line_end);
        const auto first_space = find_char(request, ' ', 0U);
        if (first_space == npos) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto second_space = find_char(request, ' ', first_space + 1U);
        if (second_space == npos || second_space + 1U >= request.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        HttpRequestView out{
            .method = method(request.first(first_space)),
            .target = request.subspan(first_space + 1U, second_space - first_space - 1U),
            .version = request.subspan(second_space + 1U),
            .headers = {},
            .body = in.subspan(head_end + 4U),
        };

        if (!http_version(out.version) || out.target.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto count = std::size_t{0U};
        auto pos = line_end + 2U;
        while (pos < head_end) {
            const auto next = find_crlf(in, pos, head_end + 2U);
            if (next == npos) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (count >= headers.size()) {
                return fail(ESP_ERR_NO_MEM);
            }

            const auto line = in.subspan(pos, next - pos);
            const auto colon = find_char(line, ':', 0U);
            if (colon == npos || colon == 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto name = line.first(colon);
            const auto value = line.subspan(colon + 1U);
            if (!valid_header_name(name) || !valid_header_value(value)) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            headers[count++] = {
                .name = name,
                .value = trim(value),
            };
            pos = next + 2U;
        }
        out.headers = headers.first(count);

        const auto len = content_length(out);
        if (!len) {
            return fail(len.error());
        }
        if (*len > out.body.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        out.body = out.body.first(*len);
        return out;
    }

    [[nodiscard]] static Result<std::size_t> content_length(const HttpRequestView& req) noexcept
    {
        if (!valid_span(req.headers)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto wanted = std::span<const char>{"content-length", 14U};
        auto seen = false;
        auto expected = std::size_t{0U};
        for (const auto& header : req.headers) {
            if (!valid_span(header.name) || !valid_span(header.value)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (!ascii_iequal(header.name, wanted)) {
                continue;
            }

            const auto value = parse_content_length(header.value);
            if (!value) {
                return fail(value.error());
            }
            if (seen && *value != expected) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            seen = true;
            expected = *value;
        }

        return seen ? expected : std::size_t{0U};
    }

    [[nodiscard]] static constexpr std::span<const char> path(const HttpRequestView& req) noexcept
    {
        if (!valid_span(req.target)) {
            return {};
        }
        const auto split = find_char(req.target, '?', 0U);
        return split == npos ? req.target : req.target.first(split);
    }

    [[nodiscard]] static constexpr std::span<const char> query(const HttpRequestView& req) noexcept
    {
        if (!valid_span(req.target)) {
            return {};
        }
        const auto split = find_char(req.target, '?', 0U);
        if (split == npos || split + 1U >= req.target.size()) {
            return {};
        }
        return req.target.subspan(split + 1U);
    }

    [[nodiscard]] static Result<std::span<const char>> find_query(
        const HttpRequestView& req,
        const char* const name) noexcept
    {
        if (name == nullptr || *name == '\0') {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto wanted = std::span<const char>{name, std::strlen(name)};
        auto rest = query(req);
        while (!rest.empty()) {
            const auto next = find_char(rest, '&', 0U);
            const auto pair = next == npos ? rest : rest.first(next);
            const auto eq = find_char(pair, '=', 0U);
            const auto key = eq == npos ? pair : pair.first(eq);
            const auto value = eq == npos ? std::span<const char>{} : pair.subspan(eq + 1U);
            if (span_equal(key, wanted)) {
                return value;
            }
            if (next == npos) {
                break;
            }
            rest = rest.subspan(next + 1U);
        }
        return fail(ESP_ERR_NOT_FOUND);
    }

    [[nodiscard]] static Result<std::span<const char>> decode_query(
        const std::span<char> out,
        const std::span<const char> value) noexcept
    {
        const auto decoded = Uri::decode(out, value, true);
        if (!decoded) {
            return fail(decoded.error());
        }
        return std::span<const char>{decoded->data(), decoded->size()};
    }

    [[nodiscard]] static Result<std::span<const char>> find_query(
        const HttpRequestView& req,
        const char* const name,
        const std::span<char> out) noexcept
    {
        const auto value = find_query(req, name);
        if (!value) {
            return fail(value.error());
        }
        return decode_query(out, *value);
    }

    [[nodiscard]] static const HttpHeaderView* find_header(
        const HttpRequestView& req,
        const char* name) noexcept
    {
        if (name == nullptr || !valid_span(req.headers)) {
            return nullptr;
        }
        const auto wanted = std::span<const char>{name, std::strlen(name)};
        for (const auto& header : req.headers) {
            if (valid_span(header.name) && ascii_iequal(header.name, wanted)) {
                return &header;
            }
        }
        return nullptr;
    }

    [[nodiscard]] static Result<std::span<const char>> text_response(
        std::span<char> out,
        int status,
        const char* reason,
        std::span<const char> body,
        const char* content_type = "text/plain") noexcept
    {
        return emit_response(out, status, reason, body, content_type);
    }

    [[nodiscard]] static Result<std::span<const char>> json_body(
        const std::span<char> out,
        const std::span<const JsonField> fields) noexcept
    {
        if (!valid_span(out) || !valid_fields(fields)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        Text text{out};
        if (!append_json(text, fields)) {
            return fail(ESP_ERR_NO_MEM);
        }
        return text.span();
    }

    [[nodiscard]] static Result<std::span<const char>> json_response(
        const std::span<char> out,
        const int status,
        const char* const reason,
        const std::span<const JsonField> fields) noexcept
    {
        if (!valid_response(status, reason, "application/json")) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        auto body = json_body(out, fields);
        if (!body) {
            return fail(body.error());
        }

        std::array<char, 160> header{};
        auto head = emit_header(std::span(header), status, reason, "application/json", body->size());
        if (!head) {
            return fail(head.error());
        }
        if (body->size() > out.size() || head->size() > out.size() - body->size()) {
            return fail(ESP_ERR_NO_MEM);
        }
        const auto total = head->size() + body->size();
        std::memmove(out.data() + head->size(), out.data(), body->size());
        std::memcpy(out.data(), head->data(), head->size());
        return out.first(total);
    }

private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    [[nodiscard]] static Result<std::span<const char>> emit_response(
        const std::span<char> out,
        const int status,
        const char* const reason,
        const std::span<const char> body,
        const char* const content_type) noexcept
    {
        if (!valid_span(out) || !valid_span(body) || !valid_response(status, reason, content_type)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto head_bytes = header_bytes(status, reason, content_type, body.size());
        if (head_bytes > out.size() || body.size() > out.size() - head_bytes) {
            return fail(ESP_ERR_NO_MEM);
        }
        if (!body.empty()) {
            std::memmove(out.data() + head_bytes, body.data(), body.size());
        }

        auto head = emit_header(out.first(head_bytes), status, reason, content_type, body.size());
        if (!head) {
            return fail(head.error());
        }
        return out.first(head->size() + body.size());
    }

    [[nodiscard]] static Result<std::span<const char>> emit_header(
        const std::span<char> out,
        const int status,
        const char* const reason,
        const char* const content_type,
        const std::size_t body_size) noexcept
    {
        if (!valid_response(status, reason, content_type)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        Text text{out};
        if (!text.append("HTTP/1.1 ") || !text.u32(static_cast<std::uint32_t>(status)) ||
            !text.push(' ') || !text.append(reason) || !text.append("\r\nContent-Type: ") ||
            !text.append(content_type) || !text.append("\r\nContent-Length: ") ||
            !text.usize(body_size) || !text.append("\r\nConnection: close\r\n\r\n")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return text.span();
    }

    [[nodiscard]] static constexpr bool valid_response(
        const int status,
        const char* const reason,
        const char* const content_type) noexcept
    {
        return status >= 100 &&
            status <= 999 &&
            valid_header_cstr(reason, true) &&
            valid_header_cstr(content_type, false);
    }

    [[nodiscard]] static constexpr std::size_t decimal_digits(std::size_t value) noexcept
    {
        auto digits = std::size_t{1U};
        while (value >= 10U) {
            value /= 10U;
            ++digits;
        }
        return digits;
    }

    [[nodiscard]] static constexpr std::size_t cstr_size(const char* const value) noexcept
    {
        auto out = std::size_t{};
        while (value[out] != '\0') {
            ++out;
        }
        return out;
    }

    [[nodiscard]] static constexpr std::size_t header_bytes(
        const int status,
        const char* const reason,
        const char* const content_type,
        const std::size_t body_size) noexcept
    {
        return (sizeof("HTTP/1.1 ") - 1U) +
            decimal_digits(static_cast<std::size_t>(status)) +
            (sizeof(" ") - 1U) +
            cstr_size(reason) +
            (sizeof("\r\nContent-Type: ") - 1U) +
            cstr_size(content_type) +
            (sizeof("\r\nContent-Length: ") - 1U) +
            decimal_digits(body_size) +
            (sizeof("\r\nConnection: close\r\n\r\n") - 1U);
    }

    [[nodiscard]] static constexpr bool header_name_char(const char ch) noexcept
    {
        return (ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            ch == '!' ||
            ch == '#' ||
            ch == '$' ||
            ch == '%' ||
            ch == '&' ||
            ch == '\'' ||
            ch == '*' ||
            ch == '+' ||
            ch == '-' ||
            ch == '.' ||
            ch == '^' ||
            ch == '_' ||
            ch == '`' ||
            ch == '|' ||
            ch == '~';
    }

    [[nodiscard]] static constexpr bool valid_header_name(const std::span<const char> value) noexcept
    {
        if (!valid_span(value) || value.empty()) {
            return false;
        }
        for (const auto ch : value) {
            if (!header_name_char(ch)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr bool valid_header_value(const std::span<const char> value) noexcept
    {
        if (!valid_span(value)) {
            return false;
        }
        for (const auto ch : value) {
            const auto byte = static_cast<unsigned char>(ch);
            if (byte != '\t' && (byte < 0x20U || byte == 0x7fU)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr bool valid_header_cstr(
        const char* const value,
        const bool allow_empty) noexcept
    {
        if (value == nullptr) {
            return false;
        }
        auto used = std::size_t{0U};
        while (value[used] != '\0') {
            const auto byte = static_cast<unsigned char>(value[used]);
            if (byte != '\t' && (byte < 0x20U || byte == 0x7fU)) {
                return false;
            }
            ++used;
        }
        return allow_empty || used != 0U;
    }

    [[nodiscard]] static Result<std::size_t> parse_content_length(
        const std::span<const char> value) noexcept
    {
        if (!valid_span(value) || value.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto out = std::size_t{0U};
        for (const auto ch : value) {
            if (ch < '0' || ch > '9') {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto digit = static_cast<std::size_t>(ch - '0');
            if (out > ((static_cast<std::size_t>(-1) - digit) / 10U)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            out = (out * 10U) + digit;
        }
        return out;
    }

    template <typename T>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static bool valid_text(const std::string_view value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static bool valid_fields(const std::span<const JsonField> fields) noexcept
    {
        if (!fields.empty() && fields.data() == nullptr) {
            return false;
        }
        for (const auto& field : fields) {
            if (!valid_text(field.name) || !valid_text(field.value.text)) {
                return false;
            }
            if (field.value.kind == JsonKind::raw && field.value.text.empty()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool append_json(Text& text, const std::span<const JsonField> fields) noexcept
    {
        if (!text.push('{')) {
            return false;
        }
        for (std::size_t i = 0U; i < fields.size(); ++i) {
            const auto& field = fields[i];
            if ((i != 0U && !text.push(',')) || !text.push('"') || !text.json(field.name) ||
                !text.append("\":") || !append_json(text, field.value)) {
                return false;
            }
        }
        return text.push('}');
    }

    [[nodiscard]] static bool append_json(Text& text, const Json value) noexcept
    {
        switch (value.kind) {
            case JsonKind::null:
                return text.append("null");
            case JsonKind::boolean:
                return text.append(value.flag ? "true" : "false");
            case JsonKind::u64:
                return text.u64(value.u);
            case JsonKind::i64:
                return text.i64(value.i);
            case JsonKind::str:
                return text.push('"') && text.json(value.text) && text.push('"');
            case JsonKind::raw:
                return text.append(value.text);
        }
        return false;
    }

    [[nodiscard]] static constexpr std::span<const char> trim(std::span<const char> value) noexcept
    {
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value = value.subspan(1U);
        }
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
            value = value.first(value.size() - 1U);
        }
        return value;
    }

    [[nodiscard]] static constexpr std::size_t find_char(
        std::span<const char> value,
        char needle,
        std::size_t from) noexcept
    {
        for (std::size_t i = from; i < value.size(); ++i) {
            if (value[i] == needle) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] static constexpr std::size_t find_crlf(
        std::span<const char> value,
        std::size_t from,
        std::size_t limit) noexcept
    {
        if (limit > value.size()) {
            limit = value.size();
        }
        if (limit < 2U || from >= limit - 1U) {
            return npos;
        }
        for (std::size_t i = from; i < limit - 1U; ++i) {
            if (value[i] == '\r' && value[i + 1U] == '\n') {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] static constexpr std::size_t headers_end(std::span<const char> value) noexcept
    {
        if (value.size() < 4U) {
            return npos;
        }
        for (std::size_t i = 0; i < value.size() - 3U; ++i) {
            if (value[i] == '\r' && value[i + 1U] == '\n' && value[i + 2U] == '\r' && value[i + 3U] == '\n') {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] static constexpr bool lit_equal(std::span<const char> value, const char* lit) noexcept
    {
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (lit[i] == '\0' || value[i] != lit[i]) {
                return false;
            }
        }
        return lit[value.size()] == '\0';
    }

    [[nodiscard]] static constexpr HttpMethod method(std::span<const char> value) noexcept
    {
        if (lit_equal(value, "GET")) {
            return HttpMethod::get;
        }
        if (lit_equal(value, "HEAD")) {
            return HttpMethod::head;
        }
        if (lit_equal(value, "POST")) {
            return HttpMethod::post;
        }
        if (lit_equal(value, "PUT")) {
            return HttpMethod::put;
        }
        if (lit_equal(value, "PATCH")) {
            return HttpMethod::patch;
        }
        if (lit_equal(value, "DELETE")) {
            return HttpMethod::del;
        }
        if (lit_equal(value, "OPTIONS")) {
            return HttpMethod::options;
        }
        return HttpMethod::unknown;
    }

    [[nodiscard]] static constexpr bool http_version(std::span<const char> value) noexcept
    {
        return value.size() == 8U && value[0] == 'H' && value[1] == 'T' && value[2] == 'T' &&
            value[3] == 'P' && value[4] == '/' && value[5] == '1' && value[6] == '.' &&
            (value[7] == '0' || value[7] == '1');
    }

    [[nodiscard]] static constexpr char lower(char ch) noexcept
    {
        return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
    }

    [[nodiscard]] static constexpr bool ascii_iequal(std::span<const char> lhs, std::span<const char> rhs) noexcept
    {
        if (!span_equal(lhs, rhs, [](const char lhs_ch, const char rhs_ch) {
                return lower(lhs_ch) == lower(rhs_ch);
            })) {
            return false;
        }
        return true;
    }

    template <typename Eq>
    [[nodiscard]] static constexpr bool span_equal(
        const std::span<const char> lhs,
        const std::span<const char> rhs,
        Eq&& eq) noexcept(noexcept(eq(lhs[0], rhs[0])))
    {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (!eq(lhs[i], rhs[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static constexpr bool span_equal(
        const std::span<const char> lhs,
        const std::span<const char> rhs) noexcept
    {
        return span_equal(lhs, rhs, [](const char lhs_ch, const char rhs_ch) {
            return lhs_ch == rhs_ch;
        });
    }
};

}  // namespace arc::net
