#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "esp_err.h"

#include "arc/result.hpp"

namespace arc {

struct Text {
    constexpr Text() noexcept = default;

    explicit constexpr Text(const std::span<char> storage) noexcept
        : out_(storage)
    {
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return pos_;
    }

    [[nodiscard]] constexpr std::size_t cap() const noexcept
    {
        return out_.size();
    }

    [[nodiscard]] constexpr std::size_t space() const noexcept
    {
        return out_.size() - pos_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return pos_ == 0U;
    }

    [[nodiscard]] constexpr bool full() const noexcept
    {
        return pos_ == out_.size();
    }

    [[nodiscard]] constexpr std::span<const char> span() const noexcept
    {
        return out_.first(pos_);
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {pos_ == 0U ? "" : out_.data(), pos_};
    }

    [[nodiscard]] constexpr Result<std::span<const char>> done() const noexcept
    {
        return span();
    }

    constexpr void clear() noexcept
    {
        pos_ = 0U;
    }

    [[nodiscard]] bool push(const char ch) noexcept
    {
        if (pos_ >= out_.size() || out_.data() == nullptr) {
            return false;
        }
        out_[pos_++] = ch;
        return true;
    }

    [[nodiscard]] bool append(const std::span<const char> value) noexcept
    {
        if (value.empty()) {
            return true;
        }
        if (value.data() == nullptr || out_.data() == nullptr) {
            return false;
        }
        if (space() < value.size()) {
            return false;
        }
        std::memmove(out_.data() + pos_, value.data(), value.size());
        pos_ += value.size();
        return true;
    }

    [[nodiscard]] bool append(const std::string_view value) noexcept
    {
        return append(std::span<const char>{value.data(), value.size()});
    }

    [[nodiscard]] bool append(const char* const value) noexcept
    {
        if (value == nullptr) {
            return false;
        }
        return append(std::string_view{value, std::strlen(value)});
    }

    [[nodiscard]] bool u32(const std::uint32_t value) noexcept
    {
        return u64(value);
    }

    [[nodiscard]] bool u64(std::uint64_t value) noexcept
    {
        if (out_.data() == nullptr) {
            return false;
        }

        char digits[20]{};
        std::size_t count{};
        do {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);

        if (space() < count) {
            return false;
        }
        while (count != 0U) {
            out_[pos_++] = digits[--count];
        }
        return true;
    }

    [[nodiscard]] bool usize(const std::size_t value) noexcept
    {
        return u64(static_cast<std::uint64_t>(value));
    }

    [[nodiscard]] bool i64(const std::int64_t value) noexcept
    {
        if (value >= 0) {
            return u64(static_cast<std::uint64_t>(value));
        }
        const auto mag = static_cast<std::uint64_t>(-(value + 1)) + 1U;
        const auto start = pos_;
        if (!push('-') || !u64(mag)) {
            pos_ = start;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool i32(const std::int32_t value) noexcept
    {
        return i64(value);
    }

    [[nodiscard]] bool hex(std::uint64_t value, const std::uint8_t width = 0U) noexcept
    {
        if (width > 16U || out_.data() == nullptr) {
            return false;
        }

        char digits[16]{};
        std::size_t count{};
        do {
            digits[count++] = hex_digit(static_cast<std::uint8_t>(value & 0x0fU));
            value >>= 4U;
        } while (value != 0U && count < sizeof(digits));

        while (count < width) {
            digits[count++] = '0';
        }

        if (space() < count) {
            return false;
        }
        while (count != 0U) {
            out_[pos_++] = digits[--count];
        }
        return true;
    }

    [[nodiscard]] bool json(const std::string_view value) noexcept
    {
        if (!value.empty() && value.data() == nullptr) {
            return false;
        }
        if (!value.empty() && out_.data() == nullptr) {
            return false;
        }

        auto bytes = std::size_t{0U};
        for (const char ch : value) {
            const auto byte = static_cast<std::uint8_t>(ch);
            const auto encoded = byte < 0x20U ? (ch == '\n' || ch == '\r' || ch == '\t' ? 2U : 6U)
                                              : (ch == '"' || ch == '\\' ? 2U : 1U);
            if (bytes > space() || space() - bytes < encoded) {
                return false;
            }
            bytes += encoded;
        }

        const auto start = pos_;
        if (bytes != 0U && shifted_overlap(out_.data() + start, bytes, value.data(), value.size())) {
            return false;
        }

        auto pos = start + bytes;
        for (std::size_t i = value.size(); i != 0U; --i) {
            const auto ch = value[i - 1U];
            const auto byte = static_cast<std::uint8_t>(ch);
            switch (ch) {
                case '"':
                case '\\':
                    pos -= 2U;
                    out_[pos] = '\\';
                    out_[pos + 1U] = ch;
                    break;
                case '\n':
                    pos -= 2U;
                    out_[pos] = '\\';
                    out_[pos + 1U] = 'n';
                    break;
                case '\r':
                    pos -= 2U;
                    out_[pos] = '\\';
                    out_[pos + 1U] = 'r';
                    break;
                case '\t':
                    pos -= 2U;
                    out_[pos] = '\\';
                    out_[pos + 1U] = 't';
                    break;
                default:
                    if (byte < 0x20U) {
                        pos -= 6U;
                        out_[pos] = '\\';
                        out_[pos + 1U] = 'u';
                        out_[pos + 2U] = '0';
                        out_[pos + 3U] = '0';
                        out_[pos + 4U] = hex_digit(byte >> 4U);
                        out_[pos + 5U] = hex_digit(byte & 0x0fU);
                    } else {
                        out_[--pos] = ch;
                    }
                    break;
            }
        }
        pos_ = start + bytes;
        return true;
    }

private:
    [[nodiscard]] static constexpr char hex_digit(const std::uint8_t value) noexcept
    {
        return static_cast<char>(value < 10U ? ('0' + value) : ('a' + (value - 10U)));
    }

    [[nodiscard]] static bool shifted_overlap(
        const char* const out,
        const std::size_t out_bytes,
        const char* const in,
        const std::size_t in_bytes) noexcept
    {
        if (out_bytes == 0U || in_bytes == 0U || out == nullptr || in == nullptr || out == in) {
            return false;
        }
        const auto out_first = reinterpret_cast<std::uintptr_t>(out);
        const auto in_first = reinterpret_cast<std::uintptr_t>(in);
        if (out_bytes > UINTPTR_MAX - out_first || in_bytes > UINTPTR_MAX - in_first) {
            return true;
        }
        const auto out_last = out_first + out_bytes;
        const auto in_last = in_first + in_bytes;
        return out_first < in_last && in_first < out_last;
    }

    std::span<char> out_{};
    std::size_t pos_{};
};

struct Hex {
    std::uint64_t value{};
    std::uint8_t width{};
};

[[nodiscard]] constexpr Hex hex(const std::uint64_t value, const std::uint8_t width = 0U) noexcept
{
    return Hex{.value = value, .width = width};
}

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

template <typename T>
concept TextArg = std::same_as<std::remove_cvref_t<T>, std::string_view> ||
    std::same_as<std::remove_cvref_t<T>, Hex> ||
    std::integral<std::remove_cvref_t<T>> || std::is_convertible_v<T, const char*>;

[[nodiscard]] inline bool put_arg(Text& out, const std::string_view value) noexcept
{
    return out.append(value);
}

template <std::size_t N>
[[nodiscard]] inline bool put_arg(Text& out, const char (&value)[N]) noexcept
{
    const auto bytes = N != 0U && value[N - 1U] == '\0' ? N - 1U : N;
    return out.append(std::string_view{value, bytes});
}

[[nodiscard]] inline bool put_arg(Text& out, const char* const value) noexcept
{
    return out.append(value);
}

[[nodiscard]] inline bool put_arg(Text& out, const char value) noexcept
{
    return out.push(value);
}

[[nodiscard]] inline bool put_arg(Text& out, const bool value) noexcept
{
    return out.append(value ? "true" : "false");
}

[[nodiscard]] inline bool put_arg(Text& out, const Hex value) noexcept
{
    return out.hex(value.value, value.width);
}

template <std::integral T>
    requires(!std::same_as<std::remove_cv_t<T>, bool> && !std::same_as<std::remove_cv_t<T>, char>)
[[nodiscard]] inline bool put_arg(Text& out, const T value) noexcept
{
    if constexpr (std::is_signed_v<T>) {
        return out.i64(static_cast<std::int64_t>(value));
    } else {
        return out.u64(static_cast<std::uint64_t>(value));
    }
}

template <typename T>
    requires(!TextArg<T>)
[[nodiscard]] inline bool put_arg(Text&, const T&) noexcept
{
    static_assert(
        TextArg<T>,
        "arc::format_to argument must be string_view, char*, char array, char, bool, integral, or arc::hex(...)");
    return false;
}

template <std::size_t Index = 0U>
[[nodiscard]] inline bool put_at(Text&, const std::size_t) noexcept
{
    return false;
}

template <std::size_t Index = 0U, typename First, typename... Rest>
[[nodiscard]] inline bool put_at(
    Text& out,
    const std::size_t target,
    const First& first,
    const Rest&... rest) noexcept
{
    if (target == Index) {
        return put_arg(out, first);
    }
    if constexpr (sizeof...(Rest) == 0U) {
        return false;
    } else {
        return put_at<Index + 1U>(out, target, rest...);
    }
}

}  // namespace detail

template <typename... Args>
[[nodiscard]] inline Result<std::span<const char>> format_to(
    const std::span<char> storage,
    const std::string_view format,
    const Args&... args) noexcept
{
    if (!detail::valid_span(storage) || !detail::valid_text(format)) {
        return fail(ESP_ERR_INVALID_ARG);
    }

    Text out{storage};
    std::size_t arg{};
    std::size_t i{};
    while (i < format.size()) {
        const auto ch = format[i];
        if (ch == '{') {
            if (i + 1U >= format.size()) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto next = format[i + 1U];
            if (next == '{') {
                if (!out.push('{')) {
                    return fail(ESP_ERR_NO_MEM);
                }
                i += 2U;
                continue;
            }
            if (next != '}') {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (arg >= sizeof...(Args)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (!detail::put_at(out, arg, args...)) {
                return fail(ESP_ERR_NO_MEM);
            }
            ++arg;
            i += 2U;
            continue;
        }
        if (ch == '}') {
            if (i + 1U >= format.size() || format[i + 1U] != '}') {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (!out.push('}')) {
                return fail(ESP_ERR_NO_MEM);
            }
            i += 2U;
            continue;
        }
        if (!out.push(ch)) {
            return fail(ESP_ERR_NO_MEM);
        }
        ++i;
    }
    if (arg != sizeof...(Args)) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return out.done();
}

}  // namespace arc
