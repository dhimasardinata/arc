#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "esp_err.h"

#include "arc/result.hpp"

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
        return req.method == Method && equal(req.target, std::span<const char>{Path.value, Path.size()});
    }

private:
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
        const auto head_end = find_headers_end(in);
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

            headers[count++] = {
                .name = trim(line.first(colon)),
                .value = trim(line.subspan(colon + 1U)),
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
        const auto* const header = find_header(req, "content-length");
        if (header == nullptr) {
            return std::size_t{0U};
        }

        auto value = std::size_t{0U};
        if (header->value.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (const auto ch : header->value) {
            if (ch < '0' || ch > '9') {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto digit = static_cast<std::size_t>(ch - '0');
            if (value > ((static_cast<std::size_t>(-1) - digit) / 10U)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            value = (value * 10U) + digit;
        }
        return value;
    }

    [[nodiscard]] static const HttpHeaderView* find_header(
        const HttpRequestView& req,
        const char* name) noexcept
    {
        if (name == nullptr) {
            return nullptr;
        }
        const auto wanted = std::span<const char>{name, std::strlen(name)};
        for (const auto& header : req.headers) {
            if (ascii_iequal(header.name, wanted)) {
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
        if (reason == nullptr || content_type == nullptr || status < 100 || status > 999) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto pos = std::size_t{0U};
        if (!append(out, pos, "HTTP/1.1 ") || !append_u16(out, pos, static_cast<std::uint16_t>(status)) ||
            !append(out, pos, " ") || !append(out, pos, reason) || !append(out, pos, "\r\nContent-Type: ") ||
            !append(out, pos, content_type) || !append(out, pos, "\r\nContent-Length: ") ||
            !append_usize(out, pos, body.size()) || !append(out, pos, "\r\nConnection: close\r\n\r\n") ||
            !append(out, pos, body)) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
    }

private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

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
        for (std::size_t i = from; i + 1U < limit; ++i) {
            if (value[i] == '\r' && value[i + 1U] == '\n') {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] static constexpr std::size_t find_headers_end(std::span<const char> value) noexcept
    {
        for (std::size_t i = 0; i + 3U < value.size(); ++i) {
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
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lower(lhs[i]) != lower(rhs[i])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool append(std::span<char> out, std::size_t& pos, std::span<const char> value) noexcept
    {
        if (out.size() - pos < value.size()) {
            return false;
        }
        std::memcpy(out.data() + pos, value.data(), value.size());
        pos += value.size();
        return true;
    }

    [[nodiscard]] static bool append(std::span<char> out, std::size_t& pos, const char* value) noexcept
    {
        return append(out, pos, std::span<const char>{value, std::strlen(value)});
    }

    [[nodiscard]] static bool append_u16(std::span<char> out, std::size_t& pos, std::uint16_t value) noexcept
    {
        if (out.size() - pos < 3U) {
            return false;
        }
        out[pos++] = static_cast<char>('0' + ((value / 100U) % 10U));
        out[pos++] = static_cast<char>('0' + ((value / 10U) % 10U));
        out[pos++] = static_cast<char>('0' + (value % 10U));
        return true;
    }

    [[nodiscard]] static bool append_usize(std::span<char> out, std::size_t& pos, std::size_t value) noexcept
    {
        std::array<char, 20> digits{};
        auto count = std::size_t{0U};
        do {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);

        if (out.size() - pos < count) {
            return false;
        }
        while (count != 0U) {
            out[pos++] = digits[--count];
        }
        return true;
    }
};

}  // namespace arc::net
