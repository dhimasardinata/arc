#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

#include "esp_attr.h"

namespace arc {

template <typename T>
struct Reg {
    static_assert(sizeof(T) <= sizeof(std::uint32_t), "reg payload must fit in one 32-bit word");
    static_assert(std::is_trivially_copyable_v<T>, "reg payload must be trivially copyable");

    void write(const T value) noexcept
    {
        store_release(bits(value));
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline T read() const noexcept
    {
        return bits<T>(load_acquire());
    }

private:
    template <typename U>
    [[nodiscard]] static constexpr std::uint32_t bits(const U value) noexcept
    {
        if constexpr (sizeof(U) == sizeof(std::uint32_t)) {
            return std::bit_cast<std::uint32_t>(value);
        } else {
            std::uint32_t raw = 0;
            __builtin_memcpy(&raw, &value, sizeof(U));
            return raw;
        }
    }

    template <typename U>
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline U bits(const std::uint32_t raw) noexcept
    {
        if constexpr (sizeof(U) == sizeof(std::uint32_t)) {
            return std::bit_cast<U>(raw);
        } else {
            U value{};
            __builtin_memcpy(&value, &raw, sizeof(U));
            return value;
        }
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline std::uint32_t load_acquire() const noexcept
    {
        return __atomic_load_n(&storage_, __ATOMIC_ACQUIRE);
    }

    inline void store_release(const std::uint32_t raw) noexcept
    {
        __atomic_store_n(&storage_, raw, __ATOMIC_RELEASE);
    }

    alignas(4) std::uint32_t storage_{0};
};

}  // namespace arc
