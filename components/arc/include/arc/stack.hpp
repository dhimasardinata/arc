#pragma once

#include <concepts>
#include <cstddef>
#include <limits>
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
    if (align == 0U) {
        return std::numeric_limits<std::size_t>::max();
    }

    const auto rem = bytes % align;
    if (rem == 0U) {
        return bytes;
    }

    const auto add = align - rem;
    return bytes > std::numeric_limits<std::size_t>::max() - add
        ? std::numeric_limits<std::size_t>::max()
        : bytes + add;
}

[[nodiscard]] constexpr std::size_t add_sat(
    const std::size_t lhs,
    const std::size_t rhs) noexcept
{
    return lhs > std::numeric_limits<std::size_t>::max() - rhs
        ? std::numeric_limits<std::size_t>::max()
        : lhs + rhs;
}

[[nodiscard]] constexpr std::size_t mul_sat(
    const std::size_t lhs,
    const std::size_t rhs) noexcept
{
    if (lhs == 0U || rhs == 0U) {
        return 0U;
    }
    return lhs > std::numeric_limits<std::size_t>::max() / rhs
        ? std::numeric_limits<std::size_t>::max()
        : lhs * rhs;
}

template <std::size_t LocalBytes = 0U,
          std::size_t CalleeBytes = 0U,
          std::size_t RuntimeBytes = runtime_reserve,
          std::size_t MarginBytes = safety_margin>
[[nodiscard]] consteval std::size_t budget() noexcept
{
    auto bytes = add_sat(LocalBytes, CalleeBytes);
    bytes = add_sat(bytes, RuntimeBytes);
    bytes = add_sat(bytes, MarginBytes);
    return round_up(bytes);
}

template <typename T, std::size_t Count = 1U>
[[nodiscard]] consteval std::size_t storage() noexcept
{
    return mul_sat(sizeof(T), Count);
}

template <typename... T>
[[nodiscard]] consteval std::size_t objects() noexcept
{
    auto bytes = std::size_t{};
    ((bytes = add_sat(bytes, sizeof(T))), ...);
    return bytes;
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
