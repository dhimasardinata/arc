#pragma once

#include <cstdint>

#include "esp_err.h"
#include "esp_sleep.h"

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
};

}  // namespace arc
