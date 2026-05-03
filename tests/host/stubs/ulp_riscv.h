#pragma once

#include <cstdint>

#include "esp_err.h"
#include "ulp.h"

using ulp_riscv_wakeup_source_t = std::uint32_t;

inline constexpr ulp_riscv_wakeup_source_t ULP_RISCV_WAKEUP_SOURCE_TIMER = 0U;
inline constexpr std::uint32_t ULP_RISCV_SW_INT = 1U;
inline constexpr std::uint32_t ULP_RISCV_TRAP_INT = 2U;

struct ulp_riscv_cfg_t {
    ulp_riscv_wakeup_source_t wakeup_source{};
};

inline esp_err_t ulp_riscv_load_binary(const std::uint8_t*, std::size_t)
{
    return ESP_OK;
}

inline esp_err_t ulp_riscv_config_and_run(const ulp_riscv_cfg_t*)
{
    return ESP_OK;
}

inline esp_err_t ulp_riscv_run()
{
    return ESP_OK;
}

inline void ulp_riscv_timer_stop() {}
inline void ulp_riscv_timer_resume() {}
inline void ulp_riscv_halt() {}
inline void ulp_riscv_reset() {}

inline esp_err_t ulp_riscv_isr_register(intr_handler_t, void*, std::uint32_t)
{
    return ESP_OK;
}

inline esp_err_t ulp_riscv_isr_deregister(intr_handler_t, void*, std::uint32_t)
{
    return ESP_OK;
}
