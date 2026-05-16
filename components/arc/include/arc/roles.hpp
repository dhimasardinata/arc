#pragma once

#include <cstddef>

namespace arc {

template <typename Lane>
class Roles {
public:
    constexpr Roles() noexcept = default;

    Roles(const Roles&) = delete;
    Roles& operator=(const Roles&) = delete;
    Roles(Roles&&) = delete;
    Roles& operator=(Roles&&) = delete;

    [[nodiscard]] constexpr auto producer() noexcept
        requires requires(Lane& lane) { lane.producer(); }
    {
        return lane_.producer();
    }

    template <std::size_t Index>
    [[nodiscard]] constexpr auto producer() noexcept
        requires requires(Lane& lane) { lane.template producer<Index>(); }
    {
        return lane_.template producer<Index>();
    }

    [[nodiscard]] constexpr auto consumer() noexcept
        requires requires(Lane& lane) { lane.consumer(); }
    {
        return lane_.consumer();
    }

    [[nodiscard]] constexpr auto split() noexcept
        requires requires(Lane& lane) { lane.split(); }
    {
        return lane_.split();
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
        requires requires { Lane::cap(); }
    {
        return Lane::cap();
    }

    [[nodiscard]] static constexpr std::size_t producers() noexcept
        requires requires { Lane::producers(); }
    {
        return Lane::producers();
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Roles);
    }

private:
    Lane lane_{};
};

}  // namespace arc
