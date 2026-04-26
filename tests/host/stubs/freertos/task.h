#pragma once

#include "freertos/FreeRTOS.h"

using TaskHandle_t = void*;
using TaskFunction_t = void (*)(void*);

inline TaskHandle_t xTaskCreateStaticPinnedToCore(
    TaskFunction_t,
    const char*,
    std::uint32_t,
    void*,
    UBaseType_t,
    StackType_t*,
    StaticTask_t*,
    BaseType_t) noexcept
{
    return reinterpret_cast<TaskHandle_t>(1);
}

inline void vTaskDelete(TaskHandle_t) noexcept {}

inline TaskHandle_t xTaskGetCurrentTaskHandle() noexcept
{
    return reinterpret_cast<TaskHandle_t>(1);
}

inline BaseType_t xPortInIsrContext() noexcept
{
    return 0;
}
