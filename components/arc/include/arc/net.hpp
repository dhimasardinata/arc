#pragma once

#include <cstdint>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"

#include "arc/init.hpp"
#include "arc/soc.hpp"
#include "arc/store.hpp"

namespace arc::net {

struct Radio {
    static_assert(Soc::wifi, "arc::net::Radio requires ESP32-S3 Wi-Fi");

    static esp_err_t base() noexcept
    {
        if (__atomic_load_n(&state.closing, __ATOMIC_ACQUIRE) != 0U) {
            return ESP_ERR_INVALID_STATE;
        }

        Gate guard(state.gate);
        return base_locked();
    }

    static esp_err_t start(
        const wifi_mode_t mode = WIFI_MODE_STA,
        const wifi_ps_type_t power_save = WIFI_PS_NONE) noexcept
    {
        Gate guard(state.gate);
        return start_locked(mode, power_save);
    }

    static esp_err_t prepare(
        const wifi_mode_t mode = WIFI_MODE_STA,
        const wifi_ps_type_t power_save = WIFI_PS_NONE) noexcept
    {
        Gate guard(state.gate);
        return prepare_locked(mode, power_save);
    }

    [[nodiscard]] static esp_netif_t* sta() noexcept
    {
        if (base() != ESP_OK) {
            return nullptr;
        }
        Gate guard(state.gate);
        return state.sta;
    }

    [[nodiscard]] static esp_netif_t* ap() noexcept
    {
        if (base() != ESP_OK) {
            return nullptr;
        }
        Gate guard(state.gate);
        return ensure_ap_locked() == ESP_OK ? state.ap : nullptr;
    }

    [[nodiscard]] static esp_err_t set(
        const wifi_interface_t iface,
        const wifi_config_t& config) noexcept
    {
        Gate guard(state.gate);
        ESP_RETURN_ON_ERROR(base_locked(), "arc-radio", "radio base failed");
        if (iface == WIFI_IF_AP) {
            ESP_RETURN_ON_ERROR(ensure_ap_locked(), "arc-radio", "ap netif failed");
        } else if (iface != WIFI_IF_STA) {
            return ESP_ERR_INVALID_ARG;
        }

        auto copy = config;
        return esp_wifi_set_config(iface, &copy);
    }

    [[nodiscard]] static esp_err_t join(
        const wifi_config_t& config,
        const wifi_ps_type_t power_save = WIFI_PS_NONE,
        const bool connect = true) noexcept
    {
        Gate guard(state.gate);
        ESP_RETURN_ON_ERROR(prepare_locked(WIFI_MODE_STA, power_save), "arc-radio", "sta prepare failed");
        auto copy = config;
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &copy), "arc-radio", "sta config failed");
        ESP_RETURN_ON_ERROR(start_locked(WIFI_MODE_STA, power_save), "arc-radio", "sta start failed");
        return connect ? esp_wifi_connect() : ESP_OK;
    }

    [[nodiscard]] static esp_err_t ap(
        const wifi_config_t& config,
        const wifi_ps_type_t power_save = WIFI_PS_NONE) noexcept
    {
        Gate guard(state.gate);
        ESP_RETURN_ON_ERROR(prepare_locked(WIFI_MODE_AP, power_save), "arc-radio", "ap prepare failed");
        auto copy = config;
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &copy), "arc-radio", "ap config failed");
        return start_locked(WIFI_MODE_AP, power_save);
    }

    [[nodiscard]] static esp_err_t apsta(
        const wifi_config_t& ap_config,
        const wifi_config_t& sta_config,
        const wifi_ps_type_t power_save = WIFI_PS_NONE,
        const bool connect = true) noexcept
    {
        Gate guard(state.gate);
        ESP_RETURN_ON_ERROR(prepare_locked(WIFI_MODE_APSTA, power_save), "arc-radio", "apsta prepare failed");
        auto ap_copy = ap_config;
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_copy), "arc-radio", "ap config failed");
        auto sta_copy = sta_config;
        ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_copy), "arc-radio", "sta config failed");
        ESP_RETURN_ON_ERROR(start_locked(WIFI_MODE_APSTA, power_save), "arc-radio", "apsta start failed");
        return connect ? esp_wifi_connect() : ESP_OK;
    }

    [[nodiscard]] static bool started() noexcept
    {
        Gate guard(state.gate);
        return state.started;
    }

    [[nodiscard]] static esp_err_t lease() noexcept
    {
        if (__atomic_load_n(&state.closing, __ATOMIC_ACQUIRE) != 0U) {
            return ESP_ERR_INVALID_STATE;
        }

        __atomic_add_fetch(&state.clients, 1U, __ATOMIC_ACQ_REL);

        if (__atomic_load_n(&state.closing, __ATOMIC_ACQUIRE) != 0U) {
            release();
            return ESP_ERR_INVALID_STATE;
        }

        const auto err = base();
        if (err != ESP_OK) {
            release();
        }
        return err;
    }

    static void release() noexcept
    {
        auto current = __atomic_load_n(&state.clients, __ATOMIC_ACQUIRE);
        while (current != 0U) {
            auto next = current - 1U;
            if (__atomic_compare_exchange_n(
                    &state.clients,
                    &current,
                    next,
                    false,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_ACQUIRE)) {
                return;
            }
        }
    }

    [[nodiscard]] static std::uint32_t leases() noexcept
    {
        return __atomic_load_n(&state.clients, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static esp_err_t stop() noexcept
    {
        Gate guard(state.gate);
        return stop_locked();
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        auto expected = std::uint32_t{};
        if (!__atomic_compare_exchange_n(
                &state.closing,
                &expected,
                1U,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return ESP_ERR_INVALID_STATE;
        }

        if (leases() != 0U) {
            __atomic_store_n(&state.closing, 0U, __ATOMIC_RELEASE);
            return ESP_ERR_INVALID_STATE;
        }

        Gate guard(state.gate);
        const auto stop_err = stop_locked();
        if (stop_err != ESP_OK) {
            __atomic_store_n(&state.closing, 0U, __ATOMIC_RELEASE);
            return stop_err;
        }

        if (!state.wifi) {
            state.prepared = false;
            state.mode = WIFI_MODE_NULL;
            state.power_save = WIFI_PS_NONE;
            if (state.sta_owner) {
                esp_netif_destroy_default_wifi(state.sta);
            }
            if (state.ap_owner) {
                esp_netif_destroy_default_wifi(state.ap);
            }
            state.sta = nullptr;
            state.sta_owner = false;
            state.ap = nullptr;
            state.ap_owner = false;
            Init::fail(state.base);
            __atomic_store_n(&state.closing, 0U, __ATOMIC_RELEASE);
            return ESP_OK;
        }

        const auto err = esp_wifi_deinit();
        if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
            __atomic_store_n(&state.closing, 0U, __ATOMIC_RELEASE);
            return err;
        }

        state.wifi = false;
        state.prepared = false;
        state.started = false;
        state.mode = WIFI_MODE_NULL;
        state.power_save = WIFI_PS_NONE;

        if (state.sta_owner) {
            esp_netif_destroy_default_wifi(state.sta);
        }
        if (state.ap_owner) {
            esp_netif_destroy_default_wifi(state.ap);
        }
        state.sta = nullptr;
        state.sta_owner = false;
        state.ap = nullptr;
        state.ap_owner = false;
        Init::fail(state.base);
        __atomic_store_n(&state.closing, 0U, __ATOMIC_RELEASE);
        return ESP_OK;
    }

private:
    static esp_err_t base_locked() noexcept
    {
        if (!Init::begin(state.base)) {
            return ESP_OK;
        }

        const auto store = Store::boot();
        if (store != ESP_OK) {
            Init::fail(state.base);
            return store;
        }

        const auto netif = esp_netif_init();
        if (netif != ESP_OK && netif != ESP_ERR_INVALID_STATE) {
            Init::fail(state.base);
            return netif;
        }

        const auto loop = esp_event_loop_create_default();
        if (loop != ESP_OK && loop != ESP_ERR_INVALID_STATE) {
            Init::fail(state.base);
            return loop;
        }

        if (state.sta == nullptr) {
            state.sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            state.sta_owner = false;
            if (state.sta == nullptr) {
                state.sta = esp_netif_create_default_wifi_sta();
                state.sta_owner = state.sta != nullptr;
            }
        }

        if (state.sta == nullptr) {
            Init::fail(state.base);
            return ESP_ERR_NO_MEM;
        }

        if (!state.wifi) {
            wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
            const auto err = esp_wifi_init(&init);
            if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
                Init::fail(state.base);
                return err;
            }
            state.wifi = true;
        }

        Init::pass(state.base);
        return ESP_OK;
    }

    static esp_err_t prepare_locked(
        const wifi_mode_t mode,
        const wifi_ps_type_t power_save) noexcept
    {
        if (mode == WIFI_MODE_AP && power_save != WIFI_PS_NONE) {
            return ESP_ERR_INVALID_ARG;
        }
        const auto active_power_save = mode == WIFI_MODE_AP ? WIFI_PS_NONE : power_save;

        ESP_RETURN_ON_ERROR(base_locked(), "arc-radio", "radio base failed");
        if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
            ESP_RETURN_ON_ERROR(ensure_ap_locked(), "arc-radio", "ap netif failed");
        }

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

            if (state.power_save != active_power_save) {
                ESP_RETURN_ON_ERROR(esp_wifi_set_ps(active_power_save), "arc-radio", "wifi ps failed");
                state.power_save = active_power_save;
            }

            return ESP_OK;
        }

        ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), "arc-radio", "wifi storage failed");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(mode), "arc-radio", "wifi mode failed");
        if (mode != WIFI_MODE_AP) {
            ESP_RETURN_ON_ERROR(esp_wifi_set_ps(active_power_save), "arc-radio", "wifi ps failed");
        }
        state.prepared = true;
        state.mode = mode;
        state.power_save = active_power_save;
        return ESP_OK;
    }

    static esp_err_t ensure_ap_locked() noexcept
    {
        if (state.ap != nullptr) {
            return ESP_OK;
        }

        state.ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        state.ap_owner = false;
        if (state.ap == nullptr) {
            state.ap = esp_netif_create_default_wifi_ap();
            state.ap_owner = state.ap != nullptr;
        }

        return state.ap == nullptr ? ESP_ERR_NO_MEM : ESP_OK;
    }

    static esp_err_t start_locked(
        const wifi_mode_t mode,
        const wifi_ps_type_t power_save) noexcept
    {
        const auto active_power_save = mode == WIFI_MODE_AP ? WIFI_PS_NONE : power_save;
        if (!state.prepared || state.mode != mode || state.power_save != active_power_save) {
            ESP_RETURN_ON_ERROR(prepare_locked(mode, power_save), "arc-radio", "radio prepare failed");
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
        state.power_save = active_power_save;
        return ESP_OK;
    }

    static esp_err_t stop_locked() noexcept
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

    struct State {
        esp_netif_t* sta;
        esp_netif_t* ap;
        bool wifi;
        bool prepared;
        bool started;
        bool sta_owner;
        bool ap_owner;
        wifi_mode_t mode;
        wifi_ps_type_t power_save;
        std::uint32_t clients;
        std::uint32_t closing;
        std::uint32_t gate;
        std::uint32_t base;
    };

    constinit static inline State state{
        nullptr,
        nullptr,
        false,
        false,
        false,
        false,
        false,
        WIFI_MODE_NULL,
        WIFI_PS_NONE,
        0U,
        0U,
        0U,
        0U,
    };
};

}  // namespace arc::net
