#pragma once

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
        std::memcpy(out_.data() + pos_, value.data(), value.size());
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
        if (width > 16U) {
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
        const auto start = pos_;
        const auto fail = [&]() noexcept {
            pos_ = start;
            return false;
        };
        for (const char ch : value) {
            const auto byte = static_cast<std::uint8_t>(ch);
            switch (ch) {
                case '"':
                case '\\':
                    if (!push('\\') || !push(ch)) {
                        return fail();
                    }
                    break;
                case '\n':
                    if (!append("\\n")) {
                        return fail();
                    }
                    break;
                case '\r':
                    if (!append("\\r")) {
                        return fail();
                    }
                    break;
                case '\t':
                    if (!append("\\t")) {
                        return fail();
                    }
                    break;
                default:
                    if (byte < 0x20U) {
                        if (!append("\\u00") || !push(hex_digit(byte >> 4U)) || !push(hex_digit(byte & 0x0fU))) {
                            return fail();
                        }
                    } else if (!push(ch)) {
                        return fail();
                    }
                    break;
            }
        }
        return true;
    }

private:
    [[nodiscard]] static constexpr char hex_digit(const std::uint8_t value) noexcept
    {
        return static_cast<char>(value < 10U ? ('0' + value) : ('a' + (value - 10U)));
    }

    std::span<char> out_{};
    std::size_t pos_{};
};

}  // namespace arc
