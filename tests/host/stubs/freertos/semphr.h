#pragma once

#include "freertos/FreeRTOS.h"

using SemaphoreHandle_t = void*;

struct StaticSemaphore_t {
    void* pad[4];
};

inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* const buffer) noexcept
{
    return buffer;
}

inline BaseType_t xSemaphoreTake(SemaphoreHandle_t handle, TickType_t) noexcept
{
    return handle == nullptr ? pdFALSE : pdTRUE;
}

inline BaseType_t xSemaphoreGive(SemaphoreHandle_t handle) noexcept
{
    return handle == nullptr ? pdFALSE : pdTRUE;
}
