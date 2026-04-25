#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>

#include "esp_check.h"
#include "esp_eap_client.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "arc/net.hpp"

namespace arc::net {

struct Eap {
    using Fast = esp_eap_fast_config;
    using Method = esp_eap_method_t;
    using Phase2 = esp_eap_ttls_phase2_types;

    [[nodiscard]] static esp_err_t identity(const void* const data, const std::size_t bytes) noexcept
    {
        return set_bytes(&esp_eap_client_set_identity, data, bytes, identity_max);
    }

    [[nodiscard]] static esp_err_t user(const void* const data, const std::size_t bytes) noexcept
    {
        return set_bytes(&esp_eap_client_set_username, data, bytes, identity_max);
    }

    [[nodiscard]] static esp_err_t password(const void* const data, const std::size_t bytes) noexcept
    {
        return set_bytes(&esp_eap_client_set_password, data, bytes);
    }

    [[nodiscard]] static esp_err_t newpass(const void* const data, const std::size_t bytes) noexcept
    {
        return set_bytes(&esp_eap_client_set_new_password, data, bytes);
    }

    [[nodiscard]] static esp_err_t ca(const void* const data, const std::size_t bytes) noexcept
    {
        return set_bytes(&esp_eap_client_set_ca_cert, data, bytes);
    }

    [[nodiscard]] static esp_err_t cert(
        const void* const client,
        const std::size_t client_bytes,
        const void* const key,
        const std::size_t key_bytes,
        const void* const key_password = nullptr,
        const std::size_t key_password_bytes = 0U) noexcept
    {
        if (!valid_bytes(client, client_bytes) ||
            !valid_bytes(key, key_bytes) ||
            key_bytes > key_max ||
            key_password_bytes > static_cast<std::size_t>(INT_MAX) ||
            (key_password == nullptr && key_password_bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_eap_client_set_certificate_and_key(
            static_cast<const unsigned char*>(client),
            static_cast<int>(client_bytes),
            static_cast<const unsigned char*>(key),
            static_cast<int>(key_bytes),
            static_cast<const unsigned char*>(key_password),
            static_cast<int>(key_password_bytes));
    }

    [[nodiscard]] static esp_err_t pac(const void* const data, const std::size_t bytes) noexcept
    {
        if (data == nullptr || bytes > static_cast<std::size_t>(INT_MAX)) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_eap_client_set_pac_file(static_cast<const unsigned char*>(data), static_cast<int>(bytes));
    }

    [[nodiscard]] static esp_err_t fast(const Fast config) noexcept
    {
        if (config.fast_provisioning < 0 ||
            config.fast_provisioning > 2 ||
            config.fast_max_pac_list_len < 0 ||
            config.fast_max_pac_list_len >= 100) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_eap_client_set_fast_params(config);
    }

    [[nodiscard]] static esp_err_t phase2(const Phase2 value) noexcept
    {
        switch (value) {
        case ESP_EAP_TTLS_PHASE2_EAP:
        case ESP_EAP_TTLS_PHASE2_MSCHAPV2:
        case ESP_EAP_TTLS_PHASE2_MSCHAP:
        case ESP_EAP_TTLS_PHASE2_PAP:
        case ESP_EAP_TTLS_PHASE2_CHAP:
            break;
        default:
            return ESP_ERR_INVALID_ARG;
        }

        return esp_eap_client_set_ttls_phase2_method(value);
    }

    [[nodiscard]] static esp_err_t suiteb(const bool enable = true) noexcept
    {
        return esp_eap_client_set_suiteb_192bit_certification(enable);
    }

    [[nodiscard]] static esp_err_t bundle(const bool enable = true) noexcept
    {
        return esp_eap_client_use_default_cert_bundle(enable);
    }

    [[nodiscard]] static esp_err_t time(const bool disable = true) noexcept
    {
        return esp_eap_client_set_disable_time_check(disable);
    }

    [[nodiscard]] static esp_err_t domain(const char* const name) noexcept
    {
        return esp_eap_client_set_domain_name(name);
    }

    [[nodiscard]] static esp_err_t methods(const Method value) noexcept
    {
        const auto bits = static_cast<int>(value);
        if (bits == 0 || (bits & ~static_cast<int>(ESP_EAP_TYPE_ALL)) != 0) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_eap_client_set_eap_methods(value);
    }

    static void okc(const bool enable = true) noexcept
    {
        esp_wifi_set_okc_support(enable);
    }

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        return esp_wifi_sta_enterprise_enable();
    }

    [[nodiscard]] static esp_err_t disable() noexcept
    {
        return esp_wifi_sta_enterprise_disable();
    }

    static void clear() noexcept
    {
        esp_eap_client_clear_identity();
        esp_eap_client_clear_username();
        esp_eap_client_clear_password();
        esp_eap_client_clear_new_password();
        esp_eap_client_clear_ca_cert();
        esp_eap_client_clear_certificate_and_key();
    }

    [[nodiscard]] static esp_err_t join(
        const wifi_config_t& config,
        const wifi_ps_type_t power_save = WIFI_PS_NONE,
        const bool connect = true) noexcept
    {
        ESP_RETURN_ON_ERROR(Radio::prepare(WIFI_MODE_STA, power_save), "arc-eap", "sta prepare failed");
        auto copy = config;
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &copy), "arc-eap", "sta config failed");
        ESP_RETURN_ON_ERROR(enable(), "arc-eap", "eap enable failed");
        ESP_RETURN_ON_ERROR(Radio::start(WIFI_MODE_STA, power_save), "arc-eap", "sta start failed");
        return connect ? esp_wifi_connect() : ESP_OK;
    }

private:
    using Setter = esp_err_t (*)(const unsigned char*, int);
    static constexpr auto identity_max = std::size_t{128U};
    static constexpr auto key_max = std::size_t{4096U};

    [[nodiscard]] static bool valid_bytes(const void* const data, const std::size_t bytes) noexcept
    {
        return data != nullptr && bytes != 0U && bytes <= static_cast<std::size_t>(INT_MAX);
    }

    [[nodiscard]] static esp_err_t set_bytes(
        const Setter setter,
        const void* const data,
        const std::size_t bytes,
        const std::size_t max = static_cast<std::size_t>(INT_MAX)) noexcept
    {
        if (setter == nullptr || !valid_bytes(data, bytes) || bytes > max) {
            return ESP_ERR_INVALID_ARG;
        }

        return setter(static_cast<const unsigned char*>(data), static_cast<int>(bytes));
    }
};

}  // namespace arc::net
