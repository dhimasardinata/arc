#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

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

struct TopologyEdge {
    std::size_t from{};
    std::size_t to{};
    std::uint32_t signal{};
};

template <int From, int To, std::uint32_t Signal = 0U>
struct PinRoute {
    static constexpr int from = From;
    static constexpr int to = To;
    static constexpr std::uint32_t signal = Signal;
};

namespace detail {

template <typename PinSet, typename Route>
[[nodiscard]] consteval bool topology_route_ok() noexcept
{
    return PinSet::template has<Route::from>() && PinSet::template has<Route::to>() &&
        Route::from != Route::to;
}

template <typename PinSet, typename Route>
[[nodiscard]] consteval TopologyEdge topology_edge() noexcept
{
    static_assert(topology_route_ok<PinSet, Route>(),
                  "[ARC ERROR] arc::TopologyGraph route references a missing or duplicate pin. Action: keep routes inside Board::pins.");
    return TopologyEdge{
        .from = PinSet::template index<Route::from>(),
        .to = PinSet::template index<Route::to>(),
        .signal = Route::signal,
    };
}

}  // namespace detail

template <Topology Board>
struct TopologyManifest {
    using PinSet = typename Board::pins;

    static constexpr std::size_t pin_count = PinSet::count;
    static constexpr auto pins = PinSet::values;
};

template <Topology Board, typename... Routes>
struct TopologyGraph {
    using PinSet = typename Board::pins;

    static constexpr std::size_t pin_count = PinSet::count;
    static constexpr std::size_t edge_count = sizeof...(Routes);
    static constexpr auto pins = PinSet::values;
    static constexpr std::array<TopologyEdge, edge_count> edges{detail::topology_edge<PinSet, Routes>()...};

    [[nodiscard]] static consteval bool valid() noexcept
    {
        return (detail::topology_route_ok<PinSet, Routes>() && ...);
    }
};

template <Topology Board>
[[nodiscard]] consteval auto topology_manifest() noexcept
{
    return TopologyManifest<Board>{};
}

template <Topology Board, typename... Routes>
[[nodiscard]] consteval auto topology_graph() noexcept
{
    return TopologyGraph<Board, Routes...>{};
}

}  // namespace arc
