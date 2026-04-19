#pragma once

#include <cerrno>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "arc/store.hpp"
#include "arc/task.hpp"

namespace arc::net {

template <typename Policy, typename Bus>
concept UdpStart = requires(Bus& bus) {
    { Policy::start(bus) } -> std::same_as<void>;
};

template <typename Policy, typename Bus>
concept UdpTick = requires(Bus& bus, const TickType_t now) {
    { Policy::tick(bus, now) } -> std::same_as<void>;
};

template <typename Policy, typename Bus>
struct Udp {
    using Event = typename Bus::event_type;

    static void boot(Bus& bus)
    {
        const auto handle = spawn(
            &run,
            "udp",
            &bus,
            2,
            Core::core0,
            stack);

        configASSERT(handle != nullptr);
    }

private:
    inline constexpr static EventBits_t wifi_up = BIT0;

    [[nodiscard]] static consteval TickType_t idle_default() noexcept
    {
        if constexpr (requires { Policy::idle; }) {
            return Policy::idle;
        } else {
            return pdMS_TO_TICKS(1);
        }
    }

    [[nodiscard]] static consteval TickType_t retry_default() noexcept
    {
        if constexpr (requires { Policy::retry; }) {
            return Policy::retry;
        } else {
            return pdMS_TO_TICKS(1000);
        }
    }

    inline constexpr static TickType_t idle_ticks = idle_default();
    inline constexpr static TickType_t retry_ticks = retry_default();

    struct State {
        StaticEventGroup_t events_mem{};
        EventGroupHandle_t events{};
        esp_event_handler_instance_t wifi_any{};
        esp_event_handler_instance_t got_ip{};
        esp_netif_t* sta{};
        int sock{-1};
        sockaddr_storage peer{};
        socklen_t peer_len{};
    };

    constinit static inline State state{};
    constinit static inline TaskMem<Policy::stack> stack{};

    static void close_socket() noexcept
    {
        if (state.sock >= 0) {
            shutdown(state.sock, 0);
            close(state.sock);
            state.sock = -1;
        }
    }

    template <std::size_t N>
    static void copy_z(std::uint8_t (&dst)[N], const char* src) noexcept
    {
        static_cast<void>(std::snprintf(reinterpret_cast<char*>(dst), N, "%s", src));
    }

    static void on_event(
        void*,
        esp_event_base_t base,
        int32_t id,
        void* data) noexcept
    {
        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
            ESP_ERROR_CHECK(esp_wifi_connect());
            return;
        }

        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupClearBits(state.events, wifi_up);
            ESP_LOGW(Policy::tag, "wifi disconnected, retrying");
            ESP_ERROR_CHECK(esp_wifi_connect());
            return;
        }

        if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
            const auto* event = static_cast<const ip_event_got_ip_t*>(data);
            ESP_LOGI(Policy::tag, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(state.events, wifi_up);
        }
    }

    static void start_wifi()
    {
        if (state.events == nullptr) {
            state.events = xEventGroupCreateStatic(&state.events_mem);
        }

        ESP_ERROR_CHECK(Store::boot());
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        state.sta = esp_netif_create_default_wifi_sta();
        configASSERT(state.sta != nullptr);

        wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&init));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &on_event,
            nullptr,
            &state.wifi_any));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &on_event,
            nullptr,
            &state.got_ip));

        wifi_config_t wifi{};
        copy_z(wifi.sta.ssid, Policy::ssid);
        copy_z(wifi.sta.password, Policy::pass);
        wifi.sta.threshold.authmode = Policy::pass[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        wifi.sta.pmf_cfg.capable = true;
        wifi.sta.pmf_cfg.required = false;

        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
        ESP_ERROR_CHECK(esp_wifi_start());

        static_cast<void>(xEventGroupWaitBits(state.events, wifi_up, pdFALSE, pdFALSE, portMAX_DELAY));
    }

    [[nodiscard]] static bool ready() noexcept
    {
        return Policy::ssid[0] != '\0' && Policy::host[0] != '\0';
    }

    [[nodiscard]] static bool open_socket() noexcept
    {
        close_socket();

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_IP;

        char port[8];
        static_cast<void>(std::snprintf(port, sizeof(port), "%u", static_cast<unsigned>(Policy::port)));

        addrinfo* res = nullptr;
        const auto err = getaddrinfo(Policy::host, port, &hints, &res);
        if (err != 0 || res == nullptr) {
            ESP_LOGE(Policy::tag, "getaddrinfo failed for %s:%s err=%d", Policy::host, port, err);
            return false;
        }

        state.sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (state.sock < 0) {
            ESP_LOGE(Policy::tag, "socket create failed errno=%d", errno);
            freeaddrinfo(res);
            return false;
        }

        const timeval timeout{
            .tv_sec = 1,
            .tv_usec = 0,
        };
        static_cast<void>(setsockopt(state.sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)));

        std::memcpy(&state.peer, res->ai_addr, res->ai_addrlen);
        state.peer_len = res->ai_addrlen;
        freeaddrinfo(res);

        ESP_LOGI(Policy::tag, "udp -> %s:%u", Policy::host, static_cast<unsigned>(Policy::port));
        return true;
    }

    [[nodiscard]] static bool send(const Event& event) noexcept
    {
        if (state.sock < 0) {
            return false;
        }

        const auto sent = sendto(
            state.sock,
            &event,
            sizeof(event),
            0,
            reinterpret_cast<const sockaddr*>(&state.peer),
            state.peer_len);

        if (sent == static_cast<int>(sizeof(event))) {
            return true;
        }

        ESP_LOGE(Policy::tag, "send failed errno=%d", errno);
        close_socket();
        return false;
    }

    static void run(void* raw) noexcept
    {
        auto& bus = *static_cast<Bus*>(raw);

        if (!ready()) {
            ESP_LOGE(Policy::tag, "set Wi-Fi and host in menuconfig");
            vTaskDelete(nullptr);
            return;
        }

        start_wifi();

        if constexpr (UdpStart<Policy, Bus>) {
            Policy::start(bus);
        }

        for (;;) {
            if ((xEventGroupGetBits(state.events) & wifi_up) == 0U) {
                close_socket();
                static_cast<void>(
                    xEventGroupWaitBits(state.events, wifi_up, pdFALSE, pdFALSE, portMAX_DELAY));
            }

            if (state.sock < 0 && !open_socket()) {
                vTaskDelay(retry_ticks);
                continue;
            }

            Event event{};
            auto sent_any = false;
            while (bus.events.try_pop(event)) {
                if (!send(event)) {
                    break;
                }
                sent_any = true;
            }

            const auto now = xTaskGetTickCount();
            if constexpr (UdpTick<Policy, Bus>) {
                Policy::tick(bus, now);
            } else {
                static_cast<void>(bus);
                static_cast<void>(now);
            }

            if (!sent_any) {
                vTaskDelay(idle_ticks);
            }
        }
    }
};

}  // namespace arc::net
