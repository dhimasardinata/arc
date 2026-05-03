#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "arc/place.hpp"

namespace arc {

template <typename T, std::size_t Capacity>
struct RtcRing {
    static_assert(Capacity > 1U, "RTC ring capacity must be greater than one");
    static_assert(std::has_single_bit(Capacity), "RTC ring capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "RTC ring payload must be trivially copyable");

    [[nodiscard]] ARC_RTC_CODE bool try_push(const T& value) noexcept
    {
        const auto current = load(&head);
        const auto next = wrap(current + 1U);
        if (next == load(&tail)) {
            return false;
        }
        data[current] = value;
        store(&head, next);
        return true;
    }

    [[nodiscard]] ARC_RTC_CODE bool try_pop(T& value) noexcept
    {
        const auto current = load(&tail);
        if (current == load(&head)) {
            return false;
        }
        value = data[current];
        store(&tail, wrap(current + 1U));
        return true;
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Capacity - 1U;
    }

    std::array<T, Capacity> data{};
    std::uint32_t head{};
    std::uint32_t tail{};

private:
    static constexpr std::uint32_t mask = static_cast<std::uint32_t>(Capacity - 1U);

    [[nodiscard]] static constexpr std::uint32_t wrap(const std::uint32_t value) noexcept
    {
        return value & mask;
    }

    [[nodiscard]] static std::uint32_t load(const std::uint32_t* value) noexcept
    {
        return __atomic_load_n(value, __ATOMIC_ACQUIRE);
    }

    static void store(std::uint32_t* slot, const std::uint32_t value) noexcept
    {
        __atomic_store_n(slot, value, __ATOMIC_RELEASE);
    }
};

}  // namespace arc
