#pragma once

#include <cstddef>
#include <cstring>

#include "esp_err.h"

#ifndef SOC_HAS
#define SOC_HAS(feature) 0
#endif

#ifndef SOC_CP_DMA_SUPPORTED
#define SOC_CP_DMA_SUPPORTED 0
#endif

struct async_memcpy_config_t {
    std::uint32_t backlog{};
    std::uint32_t weight{};
    std::size_t dma_burst_size{};
    std::uint32_t flags{};
};

struct async_memcpy_event_t {};

using async_memcpy_handle_t = void*;
using async_memcpy_isr_cb_t = bool (*)(async_memcpy_handle_t, async_memcpy_event_t*, void*);

inline esp_err_t esp_async_memcpy_install(const async_memcpy_config_t*, async_memcpy_handle_t* const out)
{
    *out = out;
    return ESP_OK;
}

inline esp_err_t esp_async_memcpy_install_gdma_ahb(const async_memcpy_config_t* config, async_memcpy_handle_t* const out)
{
    return esp_async_memcpy_install(config, out);
}

inline esp_err_t esp_async_memcpy_install_gdma_axi(const async_memcpy_config_t* config, async_memcpy_handle_t* const out)
{
    return esp_async_memcpy_install(config, out);
}

inline esp_err_t esp_async_memcpy_install_cpdma(const async_memcpy_config_t* config, async_memcpy_handle_t* const out)
{
    return esp_async_memcpy_install(config, out);
}

inline esp_err_t esp_async_memcpy(
    async_memcpy_handle_t handle,
    void* const dst,
    void* const src,
    const std::size_t bytes,
    const async_memcpy_isr_cb_t done,
    void* const user)
{
    if (handle == nullptr || dst == nullptr || src == nullptr || bytes == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    std::memcpy(dst, src, bytes);
    async_memcpy_event_t event{};
    static_cast<void>(user);
    if (done != nullptr) {
        static_cast<void>(done(handle, &event, user));
    }
    return ESP_OK;
}
