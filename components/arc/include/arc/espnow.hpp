#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "arc/store.hpp"
#include "arc/task.hpp"

namespace arc::net {

template <typename Policy, typename Bus>
concept EspNowStart = requires(Bus& bus) {
    { Policy::start(bus) } -> std::same_as<void>;
};

template <typename Policy, typename Bus>
concept EspNowTick = requires(Bus& bus, const TickType_t now) {
    { Policy::tick(bus, now) } -> std::same_as<void>;
};

template <typename Policy, typename Bus>
concept EspNowRecv = requires(Bus& bus, const std::uint8_t* peer, const std::uint8_t* data, const std::size_t size) {
    { Policy::recv(bus, peer, data, size) } -> std::same_as<void>;
};

template <typename Policy, typename Bus>
struct EspNow {
    using Event = typename Bus::event_type;

    static_assert(std::is_trivially_copyable_v<Event>, "ESP-NOW event payload must be trivially copyable");
    static_assert(sizeof(Event) <= ESP_NOW_MAX_DATA_LEN, "ESP-NOW event payload too large");
    static_assert(Policy::peer.size() == ESP_NOW_ETH_ALEN, "ESP-NOW peer must be 6 bytes");
    static_assert(Policy::channel <= 14U, "invalid ESP-NOW channel");

    static void boot(Bus& bus)
    {
        const auto handle = spawn(
            &run,
            "espnow",
            &bus,
            2,
            Core::core0,
            stack);

        configASSERT(handle != nullptr);
    }

private:
    [[nodiscard]] static consteval TickType_t idle_default() noexcept
    {
        if constexpr (requires { Policy::idle; }) {
            return Policy::idle;
        } else {
            return pdMS_TO_TICKS(1);
        }
    }

    [[nodiscard]] static consteval bool encrypt_default() noexcept
    {
        if constexpr (requires { Policy::encrypt; }) {
            return Policy::encrypt;
        } else {
            return false;
        }
    }

    inline constexpr static TickType_t idle_ticks = idle_default();

    struct State {
        Bus* bus{};
        esp_netif_t* sta{};
    };

    constinit static inline State state{};
    constinit static inline TaskMem<Policy::stack> stack{};

    template <typename Array>
    static void copy(std::uint8_t* dst, const Array& src, const std::size_t size) noexcept
    {
        std::memcpy(dst, src.data(), size);
    }

    static void start_wifi()
    {
        ESP_ERROR_CHECK(Store::boot());

        const auto netif_err = esp_netif_init();
        if (netif_err != ESP_OK && netif_err != ESP_ERR_INVALID_STATE) {
            ESP_ERROR_CHECK(netif_err);
        }

        const auto loop_err = esp_event_loop_create_default();
        if (loop_err != ESP_OK && loop_err != ESP_ERR_INVALID_STATE) {
            ESP_ERROR_CHECK(loop_err);
        }

        state.sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (state.sta == nullptr) {
            state.sta = esp_netif_create_default_wifi_sta();
        }
        configASSERT(state.sta != nullptr);

        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&init));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_ERROR_CHECK(esp_wifi_start());

        if constexpr (Policy::channel != 0U) {
            ESP_ERROR_CHECK(esp_wifi_set_channel(Policy::channel, WIFI_SECOND_CHAN_NONE));
        }
    }

    static void on_send(const esp_now_send_info_t*, const esp_now_send_status_t status) noexcept
    {
        if (status != ESP_NOW_SEND_SUCCESS) {
            ESP_LOGW(Policy::tag, "esp-now send failed");
        }
    }

    static void on_recv(
        const esp_now_recv_info_t* info,
        const std::uint8_t* data,
        const int len) noexcept
    {
        if constexpr (EspNowRecv<Policy, Bus>) {
            if (state.bus == nullptr || info == nullptr || data == nullptr || len <= 0) {
                return;
            }

            Policy::recv(
                *state.bus,
                info->src_addr,
                data,
                static_cast<std::size_t>(len));
        } else {
            static_cast<void>(info);
            static_cast<void>(data);
            static_cast<void>(len);
        }
    }

    static void start_radio()
    {
        start_wifi();

        ESP_ERROR_CHECK(esp_now_init());
        ESP_ERROR_CHECK(esp_now_register_send_cb(&on_send));

        if constexpr (EspNowRecv<Policy, Bus>) {
            ESP_ERROR_CHECK(esp_now_register_recv_cb(&on_recv));
        }

        if constexpr (requires { Policy::pmk; }) {
            static_assert(Policy::pmk.size() == ESP_NOW_KEY_LEN, "ESP-NOW PMK must be 16 bytes");
            ESP_ERROR_CHECK(esp_now_set_pmk(Policy::pmk.data()));
        }

        if (!esp_now_is_peer_exist(Policy::peer.data())) {
            esp_now_peer_info_t peer{};
            copy(peer.peer_addr, Policy::peer, ESP_NOW_ETH_ALEN);
            peer.channel = Policy::channel;
            peer.ifidx = WIFI_IF_STA;
            peer.encrypt = encrypt_default();

            if constexpr (requires { Policy::lmk; }) {
                static_assert(Policy::lmk.size() == ESP_NOW_KEY_LEN, "ESP-NOW LMK must be 16 bytes");
                copy(peer.lmk, Policy::lmk, ESP_NOW_KEY_LEN);
            }

            ESP_ERROR_CHECK(esp_now_add_peer(&peer));
        }
    }

    [[nodiscard]] static esp_err_t send(const Event& event) noexcept
    {
        return esp_now_send(
            Policy::peer.data(),
            reinterpret_cast<const std::uint8_t*>(&event),
            sizeof(event));
    }

    static void run(void* raw) noexcept
    {
        auto& bus = *static_cast<Bus*>(raw);
        state.bus = &bus;

        start_radio();

        if constexpr (EspNowStart<Policy, Bus>) {
            Policy::start(bus);
        }

        for (;;) {
            if constexpr (EspNowTick<Policy, Bus>) {
                Policy::tick(bus, xTaskGetTickCount());
            }

            Event event{};
            while (bus.events.try_pop(event)) {
                const auto err = send(event);
                if (err == ESP_OK) {
                    continue;
                }

                if (err == ESP_ERR_ESPNOW_NO_MEM) {
                    break;
                }

                ESP_LOGE(Policy::tag, "esp-now send err=0x%x", static_cast<unsigned>(err));
                break;
            }

            vTaskDelay(idle_ticks);
        }
    }
};

}  // namespace arc::net
