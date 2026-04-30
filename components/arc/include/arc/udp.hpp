#pragma once

#include <cerrno>
#include <concepts>
#include <cstdio>
#include <cstring>
#include <type_traits>

#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "arc/ip.hpp"
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

    template <auto* Shared>
    static void boot()
        requires StaticTaskState<Shared, Bus>
    {
        static_cast<void>(reap());

        auto inactive = std::uint32_t{};
        if (!__atomic_compare_exchange_n(
                &state.active,
                &inactive,
                1U,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return;
        }

        __atomic_store_n(&state.parked, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.running, 1U, __ATOMIC_RELEASE);
        const auto handle = spawn(
            &run,
            "udp",
            static_cast<void*>(Shared),
            2,
            Core::core0,
            stack);

        set_task(handle);
        if (handle == nullptr) {
            __atomic_store_n(&state.running, 0U, __ATOMIC_RELEASE);
            __atomic_store_n(&state.active, 0U, __ATOMIC_RELEASE);
            __atomic_store_n(&state.parked, 0U, __ATOMIC_RELEASE);
        }
        configASSERT(handle != nullptr);
    }

    [[nodiscard]] static esp_err_t stop(const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (__atomic_load_n(&state.active, __ATOMIC_ACQUIRE) == 0U) {
            return ESP_OK;
        }

        __atomic_store_n(&state.running, 0U, __ATOMIC_RELEASE);
        if (const auto ev = events(); ev != nullptr) {
            xEventGroupSetBits(ev, wifi_up);
        }

        if (xTaskGetCurrentTaskHandle() == task()) {
            return ESP_OK;
        }

        const auto start = xTaskGetTickCount();
        while (!reap()) {
            if (wait != portMAX_DELAY && static_cast<TickType_t>(xTaskGetTickCount() - start) >= wait) {
                return ESP_ERR_TIMEOUT;
            }
            vTaskDelay(1);
        }
        return ESP_OK;
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

    [[nodiscard]] static consteval TickType_t stale_default() noexcept
    {
        if constexpr (requires { Policy::stale; }) {
            return Policy::stale;
        } else {
            return 0;
        }
    }

    [[nodiscard]] static consteval wifi_auth_mode_t auth_default() noexcept
    {
        if constexpr (requires { Policy::auth; }) {
            return Policy::auth;
        } else {
            return Policy::pass[0] == '\0' ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        }
    }

    [[nodiscard]] static consteval bool pmf_default() noexcept
    {
        if constexpr (requires { Policy::pmf; }) {
            return Policy::pmf;
        } else {
            return false;
        }
    }

    [[nodiscard]] static consteval wifi_sae_pwe_method_t sae_default() noexcept
    {
        if constexpr (requires { Policy::sae; }) {
            return Policy::sae;
        } else {
            return WPA3_SAE_PWE_BOTH;
        }
    }

    [[nodiscard]] static consteval std::uint8_t connect_retries_default() noexcept
    {
        if constexpr (requires { Policy::connect_retries; }) {
            return Policy::connect_retries;
        } else {
            return 0U;
        }
    }

    [[nodiscard]] static consteval IpFamily family_default() noexcept
    {
        if constexpr (requires { Policy::family; }) {
            return Policy::family;
        } else {
            return IpFamily::any;
        }
    }

    inline constexpr static TickType_t idle_ticks = idle_default();
    inline constexpr static TickType_t retry_ticks = retry_default();
    inline constexpr static TickType_t stale_ticks = stale_default();
    inline constexpr static wifi_auth_mode_t auth_mode = auth_default();
    inline constexpr static bool pmf_required = pmf_default();
    inline constexpr static wifi_sae_pwe_method_t sae_pwe = sae_default();
    inline constexpr static std::uint8_t connect_retries = connect_retries_default();
    inline constexpr static IpFamily family = family_default();
    inline constexpr static bool accept_v4 = family != IpFamily::v6;
    inline constexpr static bool accept_v6 = family != IpFamily::v4;

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
        TickType_t pending_at{};
        bool have_pending{};
        bool leased{};
        bool wifi_on{};
        bool ip_on{};
        TaskHandle_t task{};
        std::uint32_t running{};
        std::uint32_t active{};
        std::uint32_t parked{};
    };

    constinit static inline State state{};
    constinit static inline TaskMem<Policy::stack, stack::required<Policy>()> stack{};

    [[nodiscard]] static EventGroupHandle_t events() noexcept
    {
        return __atomic_load_n(&state.events, __ATOMIC_ACQUIRE);
    }

    static void set_events(const EventGroupHandle_t events) noexcept
    {
        __atomic_store_n(&state.events, events, __ATOMIC_RELEASE);
    }

    [[nodiscard]] static TaskHandle_t task() noexcept
    {
        return __atomic_load_n(&state.task, __ATOMIC_ACQUIRE);
    }

    static void set_task(const TaskHandle_t task) noexcept
    {
        __atomic_store_n(&state.task, task, __ATOMIC_RELEASE);
    }

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
        if (!running()) {
            return;
        }
        const auto ev = events();
        if (ev == nullptr) {
            return;
        }

        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
            connect();
            return;
        }

        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
#if defined(CONFIG_LWIP_IPV6) && CONFIG_LWIP_IPV6
            if constexpr (accept_v6) {
                static_cast<void>(esp_netif_create_ip6_linklocal(state.sta));
            }
#endif
            return;
        }

        if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupClearBits(ev, wifi_up);
            ESP_LOGW(Policy::tag, "wifi disconnected, retrying");
            connect();
            return;
        }

        if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP && accept_v4) {
            const auto* event = static_cast<const ip_event_got_ip_t*>(data);
            if (event->esp_netif != state.sta) {
                return;
            }
            ESP_LOGI(Policy::tag, "got ip " IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(ev, wifi_up);
        }

#if defined(CONFIG_LWIP_IPV6) && CONFIG_LWIP_IPV6
        if (base == IP_EVENT && id == IP_EVENT_GOT_IP6 && accept_v6) {
            const auto* event = static_cast<const ip_event_got_ip6_t*>(data);
            if (event->esp_netif != state.sta) {
                return;
            }
            ESP_LOGI(Policy::tag, "got ip6 " IPV6STR, IPV62STR(event->ip6_info.ip));
            xEventGroupSetBits(ev, wifi_up);
        }
#endif
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

        bool ready = false;
        if constexpr (accept_v4) {
            esp_netif_ip_info_t ip{};
            ready = esp_netif_get_ip_info(state.sta, &ip) == ESP_OK && ip.ip.addr != 0U;
        }

#if defined(CONFIG_LWIP_IPV6) && CONFIG_LWIP_IPV6
        if constexpr (accept_v6) {
            esp_ip6_addr_t ip6[CONFIG_LWIP_IPV6_NUM_ADDRESSES]{};
            ready = ready || esp_netif_get_all_ip6(state.sta, ip6) > 0;
        }
#endif

        if (ready) {
            if (const auto ev = events(); ev != nullptr) {
                xEventGroupSetBits(ev, wifi_up);
            }
        }
    }

    [[nodiscard]] static bool start_wifi()
    {
        auto ev = events();
        if (ev == nullptr) {
            ev = xEventGroupCreateStatic(&state.events_mem);
            set_events(ev);
        }
        configASSERT(ev != nullptr);
        if (ev == nullptr) {
            return false;
        }
        xEventGroupClearBits(ev, wifi_up);

        if (!running()) {
            return false;
        }

#if !defined(CONFIG_LWIP_IPV6) || !CONFIG_LWIP_IPV6
        if constexpr (!accept_v4 && accept_v6) {
            ESP_LOGE(Policy::tag, "IPv6 UDP requires CONFIG_LWIP_IPV6");
            return false;
        }
#endif

        ESP_ERROR_CHECK(Radio::lease());
        state.leased = true;
        if (!running()) {
            return false;
        }

        state.sta = Radio::sta();
        configASSERT(state.sta != nullptr);
        ESP_ERROR_CHECK(Radio::prepare(WIFI_MODE_STA, WIFI_PS_NONE));
        if (!running()) {
            return false;
        }

        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &on_event,
            nullptr,
            &state.wifi_any));
        state.wifi_on = true;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &on_event,
            nullptr,
            &state.got_ip));
        state.ip_on = true;

        wifi_config_t wifi{};
        copy_z(wifi.sta.ssid, Policy::ssid);
        copy_z(wifi.sta.password, Policy::pass);
        wifi.sta.threshold.authmode = auth_mode;
        wifi.sta.pmf_cfg.capable = true;
        wifi.sta.pmf_cfg.required = pmf_required;
        wifi.sta.sae_pwe_h2e = sae_pwe;
        wifi.sta.failure_retry_cnt = connect_retries;

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi));
        ESP_ERROR_CHECK(Radio::start(WIFI_MODE_STA, WIFI_PS_NONE));
        connect();
        raise_if_ip_ready();

        while (running() && (xEventGroupGetBits(ev) & wifi_up) == 0U) {
            sleep(1);
        }
        return running();
    }

    [[nodiscard]] static bool ready() noexcept
    {
        return Policy::ssid[0] != '\0' && Policy::host[0] != '\0';
    }

    [[nodiscard]] static bool open_socket() noexcept
    {
        close_socket();

        addrinfo hints{};
        hints.ai_family = socket_family(family);
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

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

    [[nodiscard]] static bool pending_stale(const TickType_t now) noexcept
    {
        return stale_ticks != 0 && static_cast<TickType_t>(now - state.pending_at) >= stale_ticks;
    }

    [[nodiscard]] static bool running() noexcept
    {
        return __atomic_load_n(&state.running, __ATOMIC_ACQUIRE) != 0U;
    }

    static void sleep(const TickType_t ticks) noexcept
    {
        const auto span = ticks == 0U ? TickType_t{1} : ticks;
        const auto start = xTaskGetTickCount();
        while (running()) {
            vTaskDelay(1);
            if (span != portMAX_DELAY && static_cast<TickType_t>(xTaskGetTickCount() - start) >= span) {
                return;
            }
        }
    }

    static void unregister_events() noexcept
    {
        if (state.ip_on) {
            static_cast<void>(esp_event_handler_instance_unregister(
                IP_EVENT,
                IP_EVENT_STA_GOT_IP,
                state.got_ip));
            state.ip_on = false;
            state.got_ip = nullptr;
        }

        if (state.wifi_on) {
            static_cast<void>(esp_event_handler_instance_unregister(
                WIFI_EVENT,
                ESP_EVENT_ANY_ID,
                state.wifi_any));
            state.wifi_on = false;
            state.wifi_any = nullptr;
        }
    }

    static void cleanup() noexcept
    {
        close_socket();
        unregister_events();
        state.have_pending = false;
        state.sta = nullptr;

        if (state.leased) {
            Radio::release();
            state.leased = false;
        }

        __atomic_store_n(&state.running, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.parked, 1U, __ATOMIC_RELEASE);
    }

    [[nodiscard]] static bool reap() noexcept
    {
        if (__atomic_load_n(&state.active, __ATOMIC_ACQUIRE) == 0U) {
            return true;
        }

        auto parked = std::uint32_t{1U};
        if (!__atomic_compare_exchange_n(
                &state.parked,
                &parked,
                2U,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return false;
        }

        const auto handle = task();
        configASSERT(handle == nullptr || xTaskGetCurrentTaskHandle() != handle);
        if (handle != nullptr) {
            vTaskDelete(handle);
        }

        set_task(nullptr);
        __atomic_store_n(&state.running, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.active, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.parked, 0U, __ATOMIC_RELEASE);
        return true;
    }

    [[noreturn]] static void done() noexcept
    {
        cleanup();
        park_task();
    }

    static void run(void* raw) noexcept
    {
        auto& bus = *static_cast<Bus*>(raw);

        if (!ready()) {
            ESP_LOGE(Policy::tag, "set Wi-Fi and host in menuconfig");
            done();
        }

        if (!start_wifi()) {
            done();
        }

        if constexpr (UdpStart<Policy, Bus>) {
            Policy::start(bus);
        }

        while (running()) {
            const auto ev = events();
            configASSERT(ev != nullptr);
            if (ev == nullptr) {
                break;
            }
            if ((xEventGroupGetBits(ev) & wifi_up) == 0U) {
                close_socket();
                static_cast<void>(
                    xEventGroupWaitBits(ev, wifi_up, pdFALSE, pdFALSE, portMAX_DELAY));
            }

            if (!running()) {
                break;
            }

            if (state.sock < 0 && !open_socket()) {
                sleep(retry_ticks);
                continue;
            }

            Event event{};
            auto sent_any = false;
            const auto now = xTaskGetTickCount();
            if (state.have_pending && pending_stale(now)) {
                state.have_pending = false;
            }

            if (state.have_pending && send(state.pending)) {
                state.have_pending = false;
                sent_any = true;
            }

            while (!state.have_pending && bus.events.try_pop(event)) {
                if (!send(event)) {
                    state.pending = event;
                    state.pending_at = now;
                    state.have_pending = true;
                    break;
                }
                sent_any = true;
            }

            if constexpr (UdpTick<Policy, Bus>) {
                Policy::tick(bus, now);
            } else {
                static_cast<void>(bus);
                static_cast<void>(now);
            }

            if (!sent_any) {
                sleep(idle_ticks);
            }
        }

        done();
    }
};

}  // namespace arc::net
