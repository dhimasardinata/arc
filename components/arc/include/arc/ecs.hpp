#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace arc {

template <typename T, std::size_t Capacity>
struct Soa {
    static_assert(Capacity > 0U, "SoA capacity must be non-zero");
    static_assert(std::is_trivially_copyable_v<T>, "SoA component must be trivially copyable");

    [[nodiscard]] bool push(const T value) noexcept
    {
        if (count >= Capacity) {
            return false;
        }
        values[count++] = value;
        return true;
    }

    [[nodiscard]] std::span<T> span() noexcept
    {
        return {values.data(), count};
    }

    [[nodiscard]] std::span<const T> span() const noexcept
    {
        return {values.data(), count};
    }

    std::array<T, Capacity> values{};
    std::size_t count{};
};

template <std::size_t Capacity>
struct SwarmSoa {
    Soa<std::uint32_t, Capacity> id{};
    Soa<float, Capacity> x{};
    Soa<float, Capacity> y{};
    Soa<float, Capacity> vx{};
    Soa<float, Capacity> vy{};

    [[nodiscard]] bool push(
        const std::uint32_t node,
        const float px,
        const float py,
        const float dx,
        const float dy) noexcept
    {
        if (id.count >= Capacity) {
            return false;
        }
        return id.push(node) && x.push(px) && y.push(py) && vx.push(dx) && vy.push(dy);
    }
};

}  // namespace arc
