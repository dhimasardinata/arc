#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_attr.h"

#include "arc/fence.hpp"
#include "arc/sdk.hpp"

namespace arc {

template <typename T>
concept RcuValue = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

template <RcuValue T>
struct Rcu {
    alignas(cache_line) T slots[2]{};
    alignas(cache_line) mutable std::uint32_t active{};

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] const T& read() const noexcept
    {
        const auto index = __atomic_load_n(&active, __ATOMIC_ACQUIRE) & 1U;
        return slots[index];
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] T& staging() noexcept
    {
        const auto index = (__atomic_load_n(&active, __ATOMIC_RELAXED) ^ 1U) & 1U;
        return slots[index];
    }

    IRAM_ATTR [[gnu::always_inline]] void publish() noexcept
    {
        const auto current = __atomic_load_n(&active, __ATOMIC_RELAXED) & 1U;
        release_fence();
        __atomic_store_n(&active, current ^ 1U, __ATOMIC_RELEASE);
    }

    IRAM_ATTR [[gnu::always_inline]] void write(const T& value) noexcept
    {
        staging() = value;
        publish();
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] std::uint32_t index() const noexcept
    {
        return __atomic_load_n(&active, __ATOMIC_ACQUIRE) & 1U;
    }
};

}  // namespace arc
