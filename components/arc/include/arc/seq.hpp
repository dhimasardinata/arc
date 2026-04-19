#pragma once

#include <cstdint>
#include <type_traits>

#include "arc/fence.hpp"
#include "esp_attr.h"

namespace arc {

template <typename T>
struct SeqReg {
    static_assert(sizeof(T) > sizeof(std::uint32_t), "prefer arc::Reg<T> for one-word payloads");
    static_assert(std::is_trivially_copyable_v<T>, "seq payload must be trivially copyable");

    void write(const T& value) noexcept
    {
        const auto seq = __atomic_load_n(&seq_, __ATOMIC_RELAXED);
        __atomic_store_n(&seq_, seq + 1U, __ATOMIC_RELEASE);
        fence();
        __builtin_memcpy(&value_, &value, sizeof(T));
        fence();
        __atomic_store_n(&seq_, seq + 2U, __ATOMIC_RELEASE);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] T read() const noexcept
    {
        T value{};
        while (!try_read(value)) {
        }
        return value;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] bool try_read(T& value) const noexcept
    {
        const auto head = __atomic_load_n(&seq_, __ATOMIC_ACQUIRE);
        if ((head & 1U) != 0U) {
            return false;
        }

        __builtin_memcpy(&value, &value_, sizeof(T));
        fence();

        return head == __atomic_load_n(&seq_, __ATOMIC_ACQUIRE);
    }

private:
    alignas(4) std::uint32_t seq_{0};
    alignas(4) T value_{};
};

}  // namespace arc
