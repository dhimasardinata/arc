#pragma once

#include <array>
#include <cstddef>

namespace arc {

template <int... Values>
[[nodiscard]] consteval bool unique_pins() noexcept
{
    constexpr std::array<int, sizeof...(Values)> pins{Values...};

    for (std::size_t i = 0; i < pins.size(); ++i) {
        if (pins[i] < 0) {
            continue;
        }

        for (std::size_t j = i + 1U; j < pins.size(); ++j) {
            if (pins[i] == pins[j]) {
                return false;
            }
        }
    }

    return true;
}

template <int... Values>
struct Pins {
    static_assert(unique_pins<Values...>(), "duplicate physical pin in Arc topology");
    static constexpr std::array<int, sizeof...(Values)> values{Values...};
    static constexpr std::size_t count = sizeof...(Values);
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    [[nodiscard]] static consteval bool unique() noexcept
    {
        return unique_pins<Values...>();
    }

    template <int Pin>
    [[nodiscard]] static consteval bool has() noexcept
    {
        return Pin >= 0 && ((Values == Pin) || ...);
    }

    template <int Pin>
    [[nodiscard]] static consteval std::size_t index() noexcept
    {
        if constexpr (Pin < 0) {
            return npos;
        }

        for (std::size_t i = 0; i < values.size(); ++i) {
            if (values[i] == Pin) {
                return i;
            }
        }

        return npos;
    }
};

template <typename Board>
concept Topology = requires {
    typename Board::pins;
} && Board::pins::unique();

}  // namespace arc
