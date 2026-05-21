#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "arc/fence.hpp"
#include "arc/mask.hpp"
#include "arc/sdk.hpp"
#include "esp_attr.h"

namespace arc {

[[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool seq_before(
    const std::uint32_t value,
    const std::uint32_t target) noexcept
{
    return (value - target) >= 0x8000'0000U;
}

[[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool seq_reached(
    const std::uint32_t value,
    const std::uint32_t target) noexcept
{
    return !seq_before(value, target);
}

template <typename T>
struct alignas(cache_line) SeqReg {
    static_assert(sizeof(T) > sizeof(std::uint32_t), "prefer arc::Reg<T> for one-word payloads");
    static_assert(std::is_trivially_copyable_v<T>, "seq payload must be trivially copyable");
    static_assert(std::is_default_constructible_v<T>, "seq payload must be default constructible");

    void write(const T& value) noexcept
    {
        Critical guard;
        write_unmasked(value);
    }

    void write_unmasked(const T& value) noexcept
    {
        const auto seq = __atomic_load_n(&seq_, __ATOMIC_RELAXED);
        const auto next = seq + 2U;
        const auto slot = index(next);
        __atomic_store_n(&seq_, seq + 1U, __ATOMIC_RELEASE);
        sync_fence();
        __builtin_memcpy(&slots_[slot], &value, sizeof(T));
        release_fence();
        __atomic_store_n(&seq_, next, __ATOMIC_RELEASE);
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] T read() const noexcept
    {
        T value{};
        while (!try_read(value)) {
            pause();
        }
        return value;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] bool try_read(T& value) const noexcept
    {
        const auto head = __atomic_load_n(&seq_, __ATOMIC_ACQUIRE);
        if ((head & 1U) != 0U) {
            return false;
        }

        __builtin_memcpy(&value, &slots_[index(head)], sizeof(T));
        acquire_fence();

        return head == __atomic_load_n(&seq_, __ATOMIC_ACQUIRE);
    }

private:
    [[nodiscard]] static constexpr std::size_t index(const std::uint32_t seq) noexcept
    {
        return (seq >> 1U) & 1U;
    }

    alignas(4) std::uint32_t seq_{0};
    std::array<T, 2> slots_{};
};

}  // namespace arc
