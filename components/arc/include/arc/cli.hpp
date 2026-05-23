#pragma once

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>

#include "arc/result.hpp"

namespace arc {

template <std::size_t N>
struct FixedString {
    char value[N]{};

    consteval FixedString(const char (&text)[N]) noexcept
    {
        for (std::size_t i = 0U; i < N; ++i) {
            value[i] = text[i];
        }
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {value, N - 1U};
    }
};

template <FixedString Name, typename... Args>
struct Command {
    using Callback = Status (*)(Args...) noexcept;

    Callback callback{};

    [[nodiscard]] static constexpr std::string_view name() noexcept
    {
        return Name.view();
    }

    [[nodiscard]] static constexpr std::size_t argc() noexcept
    {
        return sizeof...(Args);
    }
};

template <typename... Commands>
struct Cli {
    static_assert(sizeof...(Commands) > 0U, "Cli needs at least one command");

    std::tuple<Commands...> commands{};

    [[nodiscard]] Status parse(const std::span<const char> line) const noexcept
    {
        if (!valid_span(line)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        std::array<std::string_view, max_args() + 1U> tokens{};
        const auto count = tokenize(line, tokens);
        if (count == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return dispatch<0U>(std::span<const std::string_view>{tokens.data(), count});
    }

private:
    template <typename T, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T, Extent> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static constexpr std::size_t max_args() noexcept
    {
        std::size_t out{};
        ((out = Commands::argc() > out ? Commands::argc() : out), ...);
        return out;
    }

    [[nodiscard]] static std::size_t tokenize(
        const std::span<const char> line,
        const std::span<std::string_view> out) noexcept
    {
        auto count = std::size_t{};
        auto pos = std::size_t{};
        while (pos < line.size()) {
            while (pos < line.size() && is_space(line[pos])) {
                ++pos;
            }
            const auto begin = pos;
            while (pos < line.size() && !is_space(line[pos])) {
                ++pos;
            }
            if (begin != pos) {
                if (count == out.size()) {
                    return 0U;
                }
                out[count++] = std::string_view{line.data() + begin, pos - begin};
            }
        }
        return count;
    }

    [[nodiscard]] static constexpr bool is_space(const char value) noexcept
    {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n';
    }

    template <std::size_t Index>
    [[nodiscard]] Status dispatch(const std::span<const std::string_view> tokens) const noexcept
    {
        if constexpr (Index == sizeof...(Commands)) {
            return Status{fail(ESP_ERR_NOT_FOUND)};
        } else {
            const auto& command = std::get<Index>(commands);
            if (tokens[0] == std::remove_cvref_t<decltype(command)>::name()) {
                return invoke(command, tokens.subspan(1U));
            }
            return dispatch<Index + 1U>(tokens);
        }
    }

    template <FixedString Name, typename... Args>
    [[nodiscard]] static Status invoke(
        const Command<Name, Args...>& command,
        const std::span<const std::string_view> args) noexcept
    {
        if (command.callback == nullptr || args.size() != sizeof...(Args)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        std::tuple<std::remove_cvref_t<Args>...> parsed{};
        if (!parse_args<0U, Args...>(args, parsed)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return std::apply(command.callback, parsed);
    }

    template <std::size_t Index, typename First, typename... Rest, typename Tuple>
    [[nodiscard]] static bool parse_args(
        const std::span<const std::string_view> args,
        Tuple& out) noexcept
    {
        if (!parse_one(args[Index], std::get<Index>(out))) {
            return false;
        }
        if constexpr (sizeof...(Rest) == 0U) {
            return true;
        } else {
            return parse_args<Index + 1U, Rest...>(args, out);
        }
    }

    template <std::size_t, typename Tuple>
    [[nodiscard]] static bool parse_args(
        const std::span<const std::string_view>,
        Tuple&) noexcept
    {
        return true;
    }

    template <typename T>
    [[nodiscard]] static bool parse_one(const std::string_view text, T& out) noexcept
    {
        if constexpr (std::is_same_v<T, std::string_view>) {
            out = text;
            return true;
        } else if constexpr (std::is_same_v<T, bool>) {
            if (text == "1" || text == "true" || text == "on") {
                out = true;
                return true;
            }
            if (text == "0" || text == "false" || text == "off") {
                out = false;
                return true;
            }
            return false;
        } else {
            const auto* begin = text.data();
            const auto* end = begin + text.size();
            const auto parsed = std::from_chars(begin, end, out);
            return parsed.ec == std::errc{} && parsed.ptr == end;
        }
    }
};

}  // namespace arc
