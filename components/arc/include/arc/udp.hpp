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

#include "arc/net.hpp"
#include "arc/soc.hpp"
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

    static_assert(Soc::wifi, "arc::net::Udp requires ESP32-S3 Wi-Fi");

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
        Event pending{};
        bool have_pending{};
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
            connect();
            return;
        }

        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupClearBits(state.events, wifi_up);
            ESP_LOGW(Policy::tag, "wifi disconnected, retrying");
            connect();
            return;
        }

        if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
            const auto* event = static_cast<const ip_event_got_ip_t*>(data);
            ESP_LOGI(Policy::tag, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(state.events, wifi_up);
        }
    }

    static void connect() noexcept
    {
        const auto err = esp_wifi_connect();
        if (err == ESP_OK || err == ESP_ERR_WIFI_STATE) {
            return;
        }

        ESP_ERROR_CHECK(err);
    }

    static void raise_if_ip_ready() noexcept
    {
        if (state.sta == nullptr) {
            return;
        }

        esp_netif_ip_info_t ip{};
        if (esp_netif_get_ip_info(state.sta, &ip) == ESP_OK && ip.ip.addr != 0U) {
            xEventGroupSetBits(state.events, wifi_up);
        }
    }

    static void start_wifi()
    {
        if (state.events == nullptr) {
            state.events = xEventGroupCreateStatic(&state.events_mem);
        }

        ESP_ERROR_CHECK(Radio::base());
        state.sta = Radio::sta();
        configASSERT(state.sta != nullptr);
        ESP_ERROR_CHECK(Radio::prepare(WIFI_MODE_STA, WIFI_PS_NONE));

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

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
        ESP_ERROR_CHECK(Radio::start(WIFI_MODE_STA, WIFI_PS_NONE));
        connect();
        raise_if_ip_ready();

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
            if (state.have_pending && send(state.pending)) {
                state.have_pending = false;
                sent_any = true;
            }

            while (!state.have_pending && bus.events.try_pop(event)) {
                if (!send(event)) {
                    state.pending = event;
                    state.have_pending = true;
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
