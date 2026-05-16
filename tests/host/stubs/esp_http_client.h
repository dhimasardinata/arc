#pragma once

#include <cstdint>

#include "esp_err.h"

enum esp_http_client_method_t {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_DELETE,
};

struct esp_http_client_config_t {
    const char* url{};
};

struct esp_http_client_stub_handle {
    const char* url{};
    esp_http_client_method_t method{HTTP_METHOD_GET};
    const char* body{};
    int body_len{};
    bool open{};
};

using esp_http_client_handle_t = esp_http_client_stub_handle*;

inline esp_http_client_stub_handle esp_http_client_stub{};

inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t* const config)
{
    if (config == nullptr) {
        return nullptr;
    }
    esp_http_client_stub = {};
    esp_http_client_stub.url = config->url;
    return &esp_http_client_stub;
}

inline esp_err_t esp_http_client_set_url(
    const esp_http_client_handle_t client,
    const char* const url)
{
    if (client == nullptr || url == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    client->url = url;
    return ESP_OK;
}

inline esp_err_t esp_http_client_set_method(
    const esp_http_client_handle_t client,
    const esp_http_client_method_t method)
{
    if (client == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    client->method = method;
    return ESP_OK;
}

inline esp_err_t esp_http_client_set_header(
    const esp_http_client_handle_t client,
    const char* const key,
    const char* const value)
{
    return client != nullptr && key != nullptr && value != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_http_client_set_post_field(
    const esp_http_client_handle_t client,
    const char* const data,
    const int len)
{
    if (client == nullptr || data == nullptr || len < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    client->body = data;
    client->body_len = len;
    return ESP_OK;
}

inline esp_err_t esp_http_client_perform(const esp_http_client_handle_t client)
{
    return client != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t esp_http_client_open(const esp_http_client_handle_t client, const int)
{
    if (client == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    client->open = true;
    return ESP_OK;
}

inline int esp_http_client_fetch_headers(const esp_http_client_handle_t client)
{
    return client != nullptr ? 0 : ESP_FAIL;
}

inline int esp_http_client_write(
    const esp_http_client_handle_t client,
    const char* const data,
    const int len)
{
    return client != nullptr && data != nullptr && len >= 0 ? len : ESP_FAIL;
}

inline int esp_http_client_read(
    const esp_http_client_handle_t client,
    char* const data,
    const int len)
{
    return client != nullptr && data != nullptr && len >= 0 ? 0 : ESP_FAIL;
}

inline int esp_http_client_get_status_code(const esp_http_client_handle_t client)
{
    return client != nullptr ? 200 : ESP_FAIL;
}

inline std::int64_t esp_http_client_get_content_length(const esp_http_client_handle_t client)
{
    return client != nullptr ? 0 : ESP_FAIL;
}

inline esp_err_t esp_http_client_close(const esp_http_client_handle_t client)
{
    if (client == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    client->open = false;
    return ESP_OK;
}

inline esp_err_t esp_http_client_cleanup(const esp_http_client_handle_t client)
{
    return client != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}
