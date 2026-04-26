#pragma once

#include <cstdint>

#include "esp_err.h"
#include "esp_sleep.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

struct Sleep {
    [[nodiscard]] static std::uint32_t causes() noexcept
    {
        return esp_sleep_get_wakeup_causes();
    }

    [[nodiscard]] static bool woke(const esp_sleep_source_t source) noexcept
    {
        return (causes() & (std::uint32_t{1} << static_cast<std::uint32_t>(source))) != 0U;
    }

    [[nodiscard]] static esp_err_t after_us(const std::uint64_t us) noexcept
    {
        return esp_sleep_enable_timer_wakeup(us);
    }

    template <std::uint64_t Us>
    [[nodiscard]] static esp_err_t after() noexcept
    {
        static_assert(Us != 0U, "sleep timer wake must be non-zero");
        return after_us(Us);
    }

    [[nodiscard]] static esp_err_t ulp() noexcept
    {
        return esp_sleep_enable_ulp_wakeup();
    }

    [[nodiscard]] static esp_err_t ext0(const gpio_num_t pin, const bool high = true) noexcept
    {
        return esp_sleep_enable_ext0_wakeup(pin, high ? 1 : 0);
    }

    template <int Pin, bool High = true>
    [[nodiscard]] static esp_err_t ext0() noexcept
    {
        static_assert(SOC_PM_SUPPORT_EXT0_WAKEUP, "EXT0 wake is not supported on this target");
        static_assert(Pin >= 0 && Pin < SOC_RTCIO_PIN_COUNT, "EXT0 wake requires an RTC GPIO");
        return ext0(static_cast<gpio_num_t>(Pin), High);
    }

    [[nodiscard]] static esp_err_t ext1(
        const std::uint64_t mask,
        const esp_sleep_ext1_wakeup_mode_t mode = ESP_EXT1_WAKEUP_ANY_HIGH) noexcept
    {
        return esp_sleep_enable_ext1_wakeup_io(mask, mode);
    }

    template <esp_sleep_ext1_wakeup_mode_t Mode = ESP_EXT1_WAKEUP_ANY_HIGH, int... Pins>
    [[nodiscard]] static esp_err_t ext1() noexcept
    {
        static_assert(SOC_PM_SUPPORT_EXT1_WAKEUP, "EXT1 wake is not supported on this target");
        static_assert(sizeof...(Pins) > 0U, "EXT1 wake needs at least one GPIO");
        return ext1(mask<Pins...>(), Mode);
    }

    [[nodiscard]] static esp_err_t ext1_off(const std::uint64_t mask = 0U) noexcept
    {
        return esp_sleep_disable_ext1_wakeup_io(mask);
    }

    template <int... Pins>
    [[nodiscard]] static esp_err_t ext1_off() noexcept
    {
        static_assert(sizeof...(Pins) > 0U, "EXT1 wake disable needs at least one GPIO");
        return ext1_off(mask<Pins...>());
    }

    [[nodiscard]] static std::uint64_t ext1_status() noexcept
    {
        return esp_sleep_get_ext1_wakeup_status();
    }

    [[nodiscard]] static esp_err_t off(const esp_sleep_source_t source = ESP_SLEEP_WAKEUP_ALL) noexcept
    {
        return esp_sleep_disable_wakeup_source(source);
    }

    [[nodiscard]] static esp_err_t keep(
        const esp_sleep_pd_domain_t domain,
        const esp_sleep_pd_option_t option = ESP_PD_OPTION_ON) noexcept
    {
        return esp_sleep_pd_config(domain, option);
    }

    [[nodiscard]] static esp_err_t auto_power(const esp_sleep_pd_domain_t domain) noexcept
    {
        return keep(domain, ESP_PD_OPTION_AUTO);
    }

    [[nodiscard]] static esp_err_t power_down(const esp_sleep_pd_domain_t domain) noexcept
    {
        return keep(domain, ESP_PD_OPTION_OFF);
    }

    [[nodiscard]] static esp_err_t light() noexcept
    {
        return esp_light_sleep_start();
    }

    [[noreturn]] static void deep() noexcept
    {
        esp_deep_sleep_start();
        __builtin_unreachable();
    }

private:
    template <int Pin>
    [[nodiscard]] static consteval std::uint64_t bit() noexcept
    {
        static_assert(Pin >= 0 && Pin < SOC_RTCIO_PIN_COUNT, "EXT1 wake requires RTC GPIOs");
        return std::uint64_t{1} << Pin;
    }

    template <int... Pins>
    [[nodiscard]] static consteval std::uint64_t mask() noexcept
    {
        return (bit<Pins>() | ...);
    }
};

}  // namespace arc
