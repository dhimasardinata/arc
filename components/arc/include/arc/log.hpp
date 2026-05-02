#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "esp_attr.h"

#include "arc/clock.hpp"
#include "arc/spsc.hpp"

namespace arc {

struct LogEvent {
    std::uint64_t tick{};
    std::uint32_t id{};
    std::uint32_t payload{};
    std::uint32_t aux{};
};

static_assert(std::is_trivially_copyable_v<LogEvent>);

template <std::size_t N>
[[nodiscard]] consteval std::uint32_t log_id(const char (&name)[N]) noexcept
{
    static_assert(N > 1U, "log id name must not be empty");
    std::uint32_t out = 2'166'136'261U;
    for (std::size_t i = 0; i < N - 1U; ++i) {
        out ^= static_cast<std::uint8_t>(name[i]);
        out *= 16'777'619U;
    }
    return out == 0U ? 1U : out;
}

template <std::size_t Capacity>
struct LogLane {
    static_assert(Capacity > 1U, "log lane capacity must be greater than one");
    static_assert(std::has_single_bit(Capacity), "log lane capacity must be a power of two");

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool push(
        const std::uint32_t id,
        const std::uint32_t payload = 0U,
        const std::uint32_t aux = 0U) noexcept
    {
        const LogEvent event{
            .tick = Clock::tick(),
            .id = id,
            .payload = payload,
            .aux = aux,
        };
        if (lane_.try_push(event)) {
            return true;
        }
        __atomic_add_fetch(&dropped_, 1U, __ATOMIC_RELAXED);
        return false;
    }

    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] inline bool pop(LogEvent& event) noexcept
    {
        return lane_.try_pop(event);
    }

    template <typename Fn>
    [[nodiscard]] inline std::size_t drain(Fn&& fn, const std::size_t max = Capacity - 1U) noexcept(noexcept(fn(std::declval<const LogEvent&>())))
    {
        LogEvent event{};
        std::size_t count{};
        while (count < max && lane_.try_pop(event)) {
            fn(event);
            ++count;
        }
        return count;
    }

    [[nodiscard]] inline std::uint32_t dropped() const noexcept
    {
        return __atomic_load_n(&dropped_, __ATOMIC_RELAXED);
    }

    [[nodiscard]] inline std::size_t size() const noexcept
    {
        return lane_.size();
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
    {
        return Spsc<LogEvent, Capacity>::cap();
    }

private:
    Spsc<LogEvent, Capacity> lane_{};
    alignas(cache_line) std::uint32_t dropped_{};
};

}  // namespace arc
