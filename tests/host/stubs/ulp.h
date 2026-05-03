#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

using ulp_insn_t = std::uint32_t;
using intr_handler_t = void (*)(void*);

inline esp_err_t ulp_set_wakeup_period(std::size_t, std::uint32_t)
{
    return ESP_OK;
}

inline esp_err_t ulp_load_binary(std::uint32_t, const std::uint8_t*, std::size_t)
{
    return ESP_OK;
}

inline esp_err_t ulp_process_macros_and_load(std::uint32_t, const ulp_insn_t*, std::size_t*)
{
    return ESP_OK;
}

inline esp_err_t ulp_run(std::uint32_t)
{
    return ESP_OK;
}

inline void ulp_timer_stop() {}
inline void ulp_timer_resume() {}

inline esp_err_t ulp_isr_register(intr_handler_t, void*)
{
    return ESP_OK;
}

inline esp_err_t ulp_isr_deregister(intr_handler_t, void*)
{
    return ESP_OK;
}
