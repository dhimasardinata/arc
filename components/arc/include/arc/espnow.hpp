#pragma once

#include <array>
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

#include "arc/net.hpp"
#include "arc/soc.hpp"
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

struct EspNowRadio {
    static esp_err_t start() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        const auto err = esp_now_init();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        Init::pass(state.init);
        return ESP_OK;
    }

    static esp_err_t stop() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = esp_now_deinit();
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        Init::fail(state.init);
        return ESP_OK;
    }

    static esp_err_t off() noexcept
    {
        return stop();
    }

private:
    struct State {
        std::uint32_t init;
    };

    constinit static inline State state{0U};
};

template <typename Policy, typename Bus>
struct EspNow {
    using Event = typename Bus::event_type;

    static_assert(Soc::wifi, "arc::net::EspNow requires ESP32-S3 Wi-Fi");
    static_assert(std::is_trivially_copyable_v<Event>, "ESP-NOW event payload must be trivially copyable");
    static_assert(sizeof(Event) <= ESP_NOW_MAX_DATA_LEN, "ESP-NOW event payload too large");
    static_assert(Policy::peer.size() == ESP_NOW_ETH_ALEN, "ESP-NOW peer must be 6 bytes");
    static_assert(Policy::channel <= 14U, "invalid ESP-NOW channel");

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
            "espnow",
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

    [[nodiscard]] static esp_err_t rotate_lmk(
        const std::array<std::uint8_t, ESP_NOW_KEY_LEN>& lmk,
        const bool encrypt = true) noexcept
    {
        if (__atomic_load_n(&state.active, __ATOMIC_ACQUIRE) == 0U) {
            return ESP_ERR_INVALID_STATE;
        }

        esp_now_peer_info_t peer{};
        copy(peer.peer_addr, Policy::peer, ESP_NOW_ETH_ALEN);
        peer.channel = Policy::channel;
        peer.ifidx = WIFI_IF_STA;
        peer.encrypt = encrypt;
        copy(peer.lmk, lmk, ESP_NOW_KEY_LEN);

        if (esp_now_is_peer_exist(Policy::peer.data())) {
            return esp_now_mod_peer(&peer);
        }
        return esp_now_add_peer(&peer);
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

    [[nodiscard]] static consteval TickType_t stale_default() noexcept
    {
        if constexpr (requires { Policy::stale; }) {
            return Policy::stale;
        } else {
            return 0;
        }
    }

    inline constexpr static TickType_t idle_ticks = idle_default();
    inline constexpr static TickType_t stale_ticks = stale_default();

    struct State {
        Bus* bus;
        esp_netif_t* sta;
        Event pending;
        TickType_t pending_at;
        bool have_pending;
        bool leased;
        bool send_on;
        bool recv_on;
        TaskHandle_t task;
        std::uint32_t running;
        std::uint32_t active;
        std::uint32_t parked;
    };

    constinit static inline State state{nullptr, nullptr, {}, 0, false, false, false, false, nullptr, 0U, 0U, 0U};
    constinit static inline TaskMem<Policy::stack, stack::required<Policy>()> stack{};

    [[nodiscard]] static TaskHandle_t task() noexcept
    {
        return __atomic_load_n(&state.task, __ATOMIC_ACQUIRE);
    }

    static void set_task(const TaskHandle_t task) noexcept
    {
        __atomic_store_n(&state.task, task, __ATOMIC_RELEASE);
    }

    template <typename Array>
    static void copy(std::uint8_t* dst, const Array& src, const std::size_t size) noexcept
    {
        std::memcpy(dst, src.data(), size);
    }

    [[nodiscard]] static bool start_wifi()
    {
        if (!running()) {
            return false;
        }

        ESP_ERROR_CHECK(Radio::lease());
        state.leased = true;
        if (!running()) {
            return false;
        }

        ESP_ERROR_CHECK(Radio::start(WIFI_MODE_STA, WIFI_PS_NONE));
        state.sta = Radio::sta();
        configASSERT(state.sta != nullptr);

        if constexpr (Policy::channel != 0U) {
            ESP_ERROR_CHECK(esp_wifi_set_channel(Policy::channel, WIFI_SECOND_CHAN_NONE));
        }

        return running();
    }

    static void on_send(const esp_now_send_info_t*, const esp_now_send_status_t status) noexcept
    {
        if (!running()) {
            return;
        }

        if (status != ESP_NOW_SEND_SUCCESS) {
            ESP_LOGW(Policy::tag, "esp-now send failed");
        }
    }

    static void on_recv(
        const esp_now_recv_info_t* info,
        const std::uint8_t* data,
        const int len) noexcept
    {
        if (!running()) {
            return;
        }

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

    [[nodiscard]] static bool start_radio()
    {
        if (!start_wifi()) {
            return false;
        }

        ESP_ERROR_CHECK(EspNowRadio::start());
        if (!running()) {
            return false;
        }

        ESP_ERROR_CHECK(esp_now_register_send_cb(&on_send));
        state.send_on = true;
        if (!running()) {
            return false;
        }

        if constexpr (EspNowRecv<Policy, Bus>) {
            ESP_ERROR_CHECK(esp_now_register_recv_cb(&on_recv));
            state.recv_on = true;
            if (!running()) {
                return false;
            }
        }

        if constexpr (requires { Policy::pmk; }) {
            static_assert(Policy::pmk.size() == ESP_NOW_KEY_LEN, "ESP-NOW PMK must be 16 bytes");
            ESP_ERROR_CHECK(esp_now_set_pmk(Policy::pmk.data()));
            if (!running()) {
                return false;
            }
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

        return running();
    }

    static bool send_or_hold(const Event& event, const TickType_t now) noexcept
    {
        const auto err = send(event);
        if (err == ESP_OK) {
            return true;
        }

        state.pending = event;
        state.pending_at = now;
        state.have_pending = true;

        if (err != ESP_ERR_ESPNOW_NO_MEM) {
            ESP_LOGE(Policy::tag, "esp-now send err=0x%x", static_cast<unsigned>(err));
        }

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

    [[nodiscard]] static esp_err_t send(const Event& event) noexcept
    {
        return esp_now_send(
            Policy::peer.data(),
            reinterpret_cast<const std::uint8_t*>(&event),
            sizeof(event));
    }

    static void cleanup() noexcept
    {
        if constexpr (EspNowRecv<Policy, Bus>) {
            if (state.recv_on) {
                static_cast<void>(esp_now_unregister_recv_cb());
                state.recv_on = false;
            }
        }

        if (state.send_on) {
            static_cast<void>(esp_now_unregister_send_cb());
            state.send_on = false;
        }

        static_cast<void>(EspNowRadio::stop());
        state.bus = nullptr;
        state.sta = nullptr;
        state.have_pending = false;

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
        state.bus = &bus;

        if (!start_radio()) {
            done();
        }

        if constexpr (EspNowStart<Policy, Bus>) {
            Policy::start(bus);
        }

        while (running()) {
            const auto now = xTaskGetTickCount();
            if constexpr (EspNowTick<Policy, Bus>) {
                Policy::tick(bus, now);
            }

            if (!running()) {
                break;
            }

            if (state.have_pending && pending_stale(now)) {
                state.have_pending = false;
            }

            if (state.have_pending && send(state.pending) == ESP_OK) {
                state.have_pending = false;
            }

            Event event{};
            while (!state.have_pending && bus.events.try_pop(event)) {
                if (!send_or_hold(event, now)) {
                    break;
                }
            }

            sleep(idle_ticks);
        }

        done();
    }
};

}  // namespace arc::net
