#pragma once

#include <cstddef>
#include <type_traits>

namespace arc::fsm {

template <auto From, auto Event, auto To>
struct Transition {
    static constexpr auto from = From;
    static constexpr auto event = Event;
    static constexpr auto to = To;
};

template <auto... States>
struct Terminals {
    template <auto State>
    static consteval bool contains() noexcept
    {
        return ((State == States) || ... || false);
    }
};

template <auto Initial, typename TerminalList, typename... Edges>
struct Automaton {
    static_assert(sizeof...(Edges) > 0U, "fsm automaton needs at least one transition");

    using State = decltype(Initial);

    template <auto StateValue, std::size_t Depth = sizeof...(Edges) + 1U>
    [[nodiscard]] static consteval bool reachable() noexcept
    {
        if constexpr (StateValue == Initial) {
            return true;
        } else if constexpr (Depth == 0U) {
            return false;
        } else {
            return ((Edges::to == StateValue &&
                     Automaton::template reachable<Edges::from, Depth - 1U>()) ||
                    ...);
        }
    }

    template <auto StateValue>
    [[nodiscard]] static consteval bool has_outgoing() noexcept
    {
        return ((Edges::from == StateValue) || ...);
    }

    [[nodiscard]] static consteval bool valid() noexcept
    {
        return (reachable<Edges::from>() && ...) &&
            (reachable<Edges::to>() && ...) &&
            ((TerminalList::template contains<Edges::to>() || has_outgoing<Edges::to>()) && ...);
    }

    template <typename Event>
    [[nodiscard]] static constexpr State dispatch(
        const State state,
        const Event event) noexcept
    {
        State next = state;
        (((state == Edges::from) && (event == Edges::event)
              ? (next = static_cast<State>(Edges::to), true)
              : false),
         ...);
        return next;
    }

    static_assert(valid(), "fsm automaton has an unreachable state or non-terminal dead end");
};

}  // namespace arc::fsm
