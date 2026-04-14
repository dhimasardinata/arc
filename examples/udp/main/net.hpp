#pragma once

#include <cerrno>
#include <cstdio>
#include <cstring>

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

#include "arc/task.hpp"
#include "cfg.hpp"
#include "lane.hpp"

namespace app::net {

inline constexpr char tag[] = "arc-udp";
inline constexpr EventBits_t wifi_up = BIT0;
inline constexpr TickType_t idle_ticks = pdMS_TO_TICKS(1);
inline constexpr TickType_t retry_ticks = pdMS_TO_TICKS(1000);

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

constinit inline State state{};
constinit inline arc::TaskMem<cfg::net_stack> mem{};

inline void close_sock() noexcept
{
    if (state.sock >= 0) {
        shutdown(state.sock, 0);
        close(state.sock);
        state.sock = -1;
    }
}

inline void init_nvs()
{
    const auto err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
        return;
    }

    ESP_ERROR_CHECK(err);
}

template <std::size_t N>
inline void copy_z(std::uint8_t (&dst)[N], const char* src) noexcept
{
    static_cast<void>(std::snprintf(reinterpret_cast<char*>(dst), N, "%s", src));
}

inline void on_event(
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
        ESP_LOGW(tag, "wifi disconnected, retrying");
        ESP_ERROR_CHECK(esp_wifi_connect());
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        const auto* event = static_cast<const ip_event_got_ip_t*>(data);
        ESP_LOGI(tag, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(state.events, wifi_up);
    }
}

inline void start_wifi()
{
    if (state.events == nullptr) {
        state.events = xEventGroupCreateStatic(&state.events_mem);
    }

    init_nvs();
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
    copy_z(wifi.sta.ssid, cfg::ssid);
    copy_z(wifi.sta.password, cfg::pass);
    wifi.sta.threshold.authmode = cfg::pass[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    wifi.sta.pmf_cfg.capable = true;
    wifi.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
    ESP_ERROR_CHECK(esp_wifi_start());

    static_cast<void>(xEventGroupWaitBits(state.events, wifi_up, pdFALSE, pdFALSE, portMAX_DELAY));
}

[[nodiscard]] inline bool open_socket() noexcept
{
    close_sock();

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_IP;

    char port[8];
    static_cast<void>(std::snprintf(port, sizeof(port), "%u", static_cast<unsigned>(cfg::port)));

    addrinfo* res = nullptr;
    const auto err = getaddrinfo(cfg::host, port, &hints, &res);
    if (err != 0 || res == nullptr) {
        ESP_LOGE(tag, "getaddrinfo failed for %s:%s err=%d", cfg::host, port, err);
        return false;
    }

    state.sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (state.sock < 0) {
        ESP_LOGE(tag, "socket create failed errno=%d", errno);
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

    ESP_LOGI(tag, "udp -> %s:%u", cfg::host, static_cast<unsigned>(cfg::port));
    return true;
}

[[nodiscard]] inline bool send(const Telemetry& sample) noexcept
{
    if (state.sock < 0) {
        return false;
    }

    const auto sent = sendto(
        state.sock,
        &sample,
        sizeof(sample),
        0,
        reinterpret_cast<const sockaddr*>(&state.peer),
        state.peer_len);

    if (sent == static_cast<int>(sizeof(sample))) {
        return true;
    }

    ESP_LOGE(tag, "send failed errno=%d", errno);
    close_sock();
    return false;
}

inline void apply(Ctx& ctx, const bool fast) noexcept
{
    const auto half_us = fast ? cfg::fast_half_us : cfg::half_us;
    const auto ctl = fast ? fast_ctl : slow_ctl;

    ctx.half.write(half_cycles(half_us));
    ctx.ctl.write(ctl);

    ESP_LOGI(
        tag,
        "cmd half=%u us mark=%u stride=%u flags=0x%x",
        static_cast<unsigned>(half_us),
        static_cast<unsigned>(ctl.mark),
        static_cast<unsigned>(ctl.stride),
        static_cast<unsigned>(ctl.flags));
}

[[nodiscard]] inline bool ready() noexcept
{
    return cfg::ssid[0] != '\0' && cfg::host[0] != '\0';
}

inline void task(void* raw) noexcept
{
    auto& ctx = *static_cast<Ctx*>(raw);

    if (!ready()) {
        ESP_LOGE(tag, "set Arc UDP SSID and host in menuconfig");
        vTaskDelete(nullptr);
        return;
    }

    start_wifi();
    apply(ctx, false);

    auto last_cmd = xTaskGetTickCount();
    auto fast = false;

    for (;;) {
        if ((xEventGroupGetBits(state.events) & wifi_up) == 0U) {
            close_sock();
            static_cast<void>(xEventGroupWaitBits(state.events, wifi_up, pdFALSE, pdFALSE, portMAX_DELAY));
        }

        if (state.sock < 0 && !open_socket()) {
            vTaskDelay(retry_ticks);
            continue;
        }

        Telemetry sample{};
        auto sent = false;
        while (ctx.tx.try_pop(sample)) {
            if (!send(sample)) {
                break;
            }
            sent = true;
        }

        const auto now = xTaskGetTickCount();
        if ((now - last_cmd) >= pdMS_TO_TICKS(cfg::cmd_ms)) {
            last_cmd = now;
            fast = !fast;
            apply(ctx, fast);
        }

        if (!sent) {
            vTaskDelay(idle_ticks);
        }
    }
}

inline void boot(Ctx& ctx)
{
    const auto handle = arc::spawn(
        &task,
        "net",
        &ctx,
        2,
        arc::Core::core0,
        mem);

    configASSERT(handle != nullptr);
}

}  // namespace app::net
