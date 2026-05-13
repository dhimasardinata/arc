#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace arc::rtos {

[[nodiscard]] constexpr std::uint32_t tick_hz() noexcept
{
    return static_cast<std::uint32_t>(configTICK_RATE_HZ);
}

[[nodiscard]] constexpr std::uint32_t milliseconds_max() noexcept
{
    return std::numeric_limits<std::uint32_t>::max();
}

template <typename Rep, typename Period>
[[nodiscard]] constexpr std::uint32_t milliseconds(
    const std::chrono::duration<Rep, Period> timeout) noexcept
{
    if (timeout <= std::chrono::duration<Rep, Period>::zero()) {
        return 0U;
    }

    const auto rounded = std::chrono::ceil<std::chrono::milliseconds>(timeout);
    const auto count = rounded.count();
    if (count <= 0) {
        return 0U;
    }

    constexpr auto max = static_cast<decltype(count)>(milliseconds_max());
    return count > max ? milliseconds_max() : static_cast<std::uint32_t>(count);
}

[[nodiscard]] constexpr TickType_t ticks_ms(const std::uint32_t timeout_ms) noexcept
{
    if (timeout_ms == 0U) {
        return 0U;
    }

    constexpr auto tick_max = static_cast<std::uint64_t>(portMAX_DELAY);
    const auto ticks =
        ((static_cast<std::uint64_t>(timeout_ms) * tick_hz()) + 999ULL) / 1000ULL;
    if (ticks > tick_max) {
        return portMAX_DELAY;
    }
    return static_cast<TickType_t>(ticks == 0U ? 1U : ticks);
}

template <typename Rep, typename Period>
[[nodiscard]] constexpr TickType_t ticks(
    const std::chrono::duration<Rep, Period> timeout) noexcept
{
    return ticks_ms(milliseconds(timeout));
}

template <typename Rep, typename Period>
void delay_for(const std::chrono::duration<Rep, Period> timeout) noexcept
{
    vTaskDelay(ticks(timeout));
}

}  // namespace arc::rtos
