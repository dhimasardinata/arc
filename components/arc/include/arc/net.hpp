#pragma once

#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "arc/soc.hpp"
#include "arc/store.hpp"

namespace arc::net {

struct Radio {
    static_assert(Soc::wifi, "arc::net::Radio requires ESP32-S3 Wi-Fi");

    static esp_err_t base() noexcept
    {
        ESP_RETURN_ON_ERROR(Store::boot(), "arc-radio", "nvs boot failed");

        const auto netif = esp_netif_init();
        if (netif != ESP_OK && netif != ESP_ERR_INVALID_STATE) {
            return netif;
        }

        const auto loop = esp_event_loop_create_default();
        if (loop != ESP_OK && loop != ESP_ERR_INVALID_STATE) {
            return loop;
        }

        if (state.sta == nullptr) {
            state.sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (state.sta == nullptr) {
                state.sta = esp_netif_create_default_wifi_sta();
            }
        }

        if (state.sta == nullptr) {
            return ESP_ERR_NO_MEM;
        }

        if (!state.wifi) {
            wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
            const auto err = esp_wifi_init(&init);
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                return err;
            }
            state.wifi = true;
        }

        return ESP_OK;
    }

    static esp_err_t start(
        const wifi_mode_t mode = WIFI_MODE_STA,
        const wifi_ps_type_t power_save = WIFI_PS_NONE) noexcept
    {
        if (!state.prepared || state.mode != mode || state.power_save != power_save) {
            ESP_RETURN_ON_ERROR(prepare(mode, power_save), "arc-radio", "radio prepare failed");
        }

        if (state.started) {
            return ESP_OK;
        }

        const auto err = esp_wifi_start();
        if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
            return err;
        }

        state.started = true;
        state.mode = mode;
        state.power_save = power_save;
        return ESP_OK;
    }

    static esp_err_t prepare(
        const wifi_mode_t mode = WIFI_MODE_STA,
        const wifi_ps_type_t power_save = WIFI_PS_NONE) noexcept
    {
        ESP_RETURN_ON_ERROR(base(), "arc-radio", "radio base failed");

        wifi_mode_t current = WIFI_MODE_NULL;
        const auto mode_err = esp_wifi_get_mode(&current);
        if (mode_err != ESP_OK && mode_err != ESP_ERR_WIFI_NOT_INIT) {
            return mode_err;
        }

        if (state.started) {
            if (current != mode) {
                return ESP_ERR_INVALID_STATE;
            }
            state.mode = current;

            if (state.power_save != power_save) {
                ESP_RETURN_ON_ERROR(esp_wifi_set_ps(power_save), "arc-radio", "wifi ps failed");
                state.power_save = power_save;
            }

            return ESP_OK;
        }

        ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), "arc-radio", "wifi storage failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(mode), "arc-radio", "wifi mode failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_ps(power_save), "arc-radio", "wifi ps failed");
        state.prepared = true;
        state.mode = mode;
        state.power_save = power_save;
        return ESP_OK;
    }

    [[nodiscard]] static esp_netif_t* sta() noexcept
    {
        static_cast<void>(base());
        return state.sta;
    }

    [[nodiscard]] static bool started() noexcept
    {
        return state.started;
    }

    [[nodiscard]] static esp_err_t stop() noexcept
    {
        if (!state.started) {
            return ESP_OK;
        }

        const auto err = esp_wifi_stop();
        if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
            return err;
        }

        state.started = false;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        ESP_RETURN_ON_ERROR(stop(), "arc-radio", "wifi stop failed");

        if (!state.wifi) {
            state.prepared = false;
            state.mode = WIFI_MODE_NULL;
            state.power_save = WIFI_PS_NONE;
            return ESP_OK;
        }

        const auto err = esp_wifi_deinit();
        if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
            return err;
        }

        state.wifi = false;
        state.prepared = false;
        state.started = false;
        state.mode = WIFI_MODE_NULL;
        state.power_save = WIFI_PS_NONE;
        return ESP_OK;
    }

private:
    struct State {
        esp_netif_t* sta;
        bool wifi;
        bool prepared;
        bool started;
        wifi_mode_t mode;
        wifi_ps_type_t power_save;
    };

    constinit static inline State state{
        nullptr,
        false,
        false,
        false,
        WIFI_MODE_NULL,
        WIFI_PS_NONE,
    };
};

}  // namespace arc::net
