#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace arc::stack {

inline constexpr std::size_t task_floor = 2048U;
inline constexpr std::size_t runtime_reserve = 512U;
inline constexpr std::size_t safety_margin = 512U;
inline constexpr std::size_t frame_limit = 1024U;

[[nodiscard]] constexpr std::size_t round_up(
    const std::size_t bytes,
    const std::size_t align = alignof(std::max_align_t)) noexcept
{
    return (bytes + align - 1U) & ~(align - 1U);
}

template <std::size_t LocalBytes = 0U,
          std::size_t CalleeBytes = 0U,
          std::size_t RuntimeBytes = runtime_reserve,
          std::size_t MarginBytes = safety_margin>
[[nodiscard]] consteval std::size_t budget() noexcept
{
    return round_up(LocalBytes + CalleeBytes + RuntimeBytes + MarginBytes);
}

template <typename T, std::size_t Count = 1U>
[[nodiscard]] consteval std::size_t storage() noexcept
{
    return sizeof(T) * Count;
}

template <typename... T>
[[nodiscard]] consteval std::size_t objects() noexcept
{
    return (std::size_t{} + ... + sizeof(T));
}

template <typename T>
concept Declared = requires {
    { T::stack_bytes } -> std::convertible_to<std::size_t>;
};

template <typename T>
[[nodiscard]] consteval std::size_t declared() noexcept
{
    if constexpr (Declared<T>) {
        return round_up(static_cast<std::size_t>(T::stack_bytes));
    } else {
        return 0U;
    }
}

template <typename Workload, typename State = void>
[[nodiscard]] consteval std::size_t required() noexcept
{
    auto bytes = declared<Workload>();
    if constexpr (!std::same_as<State, void>) {
        const auto state = declared<State>();
        bytes = state > bytes ? state : bytes;
    }
    return bytes < task_floor ? task_floor : bytes;
}

template <std::size_t StackBytes, std::size_t RequiredBytes>
[[nodiscard]] consteval bool fits() noexcept
{
    return StackBytes >= RequiredBytes;
}

}  // namespace arc::stack
