#pragma once

#include <cstddef>
#include <sys/types.h>

#include "esp_err.h"

#define ESP_TLS_ERR_SSL_WANT_READ -0x6900
#define ESP_TLS_ERR_SSL_WANT_WRITE -0x6880

struct esp_tls_cfg_t {
    const unsigned char* cacert_buf{};
    std::size_t cacert_bytes{};
};

struct esp_tls_t {
    bool connected{};
    int sock{7};
};

inline esp_tls_t esp_tls_stub{};

inline esp_tls_t* esp_tls_init()
{
    esp_tls_stub = {};
    return &esp_tls_stub;
}

inline int esp_tls_conn_new_sync(
    const char* const host,
    const int host_len,
    const int port,
    const esp_tls_cfg_t* const,
    esp_tls_t* const handle)
{
    if (host == nullptr || host_len <= 0 || port <= 0 || handle == nullptr) {
        return -1;
    }
    handle->connected = true;
    return 1;
}

inline int esp_tls_conn_destroy(esp_tls_t* const handle)
{
    if (handle == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    handle->connected = false;
    return ESP_OK;
}

inline ssize_t esp_tls_conn_write(
    esp_tls_t* const handle,
    const void* const data,
    const std::size_t bytes)
{
    if (handle == nullptr || !handle->connected || data == nullptr) {
        return -1;
    }
    return static_cast<ssize_t>(bytes);
}

inline ssize_t esp_tls_conn_read(
    esp_tls_t* const handle,
    void* const data,
    const std::size_t)
{
    if (handle == nullptr || !handle->connected || data == nullptr) {
        return -1;
    }
    return 0;
}

inline int esp_tls_get_bytes_avail(esp_tls_t* const handle)
{
    return handle != nullptr && handle->connected ? 0 : -1;
}

inline esp_err_t esp_tls_get_conn_sockfd(esp_tls_t* const handle, int* const sock)
{
    if (handle == nullptr || !handle->connected || sock == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *sock = handle->sock;
    return ESP_OK;
}
