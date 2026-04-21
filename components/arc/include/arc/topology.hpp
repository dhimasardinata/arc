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

    [[nodiscard]] static consteval bool unique() noexcept
    {
        return unique_pins<Values...>();
    }
};

template <typename Board>
concept Topology = requires {
    typename Board::pins;
} && Board::pins::unique();

}  // namespace arc
