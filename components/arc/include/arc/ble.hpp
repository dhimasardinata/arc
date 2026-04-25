#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_hs_id.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "services/gap/ble_svc_gap.h"

#include "arc/init.hpp"
#include "arc/soc.hpp"
#include "arc/store.hpp"
#include "arc/task.hpp"

namespace arc {

struct Ble {
    static_assert(Soc::ble, "BLE is not supported on this target");

    using Addr = std::array<std::uint8_t, 6>;

    static constexpr std::int32_t forever = BLE_HS_FOREVER;

    [[nodiscard]] static esp_err_t init() noexcept
    {
        Gate guard(state.gate);
        if (state.ready) {
            return ESP_OK;
        }

        const auto store = Store::boot();
        if (store != ESP_OK) {
            return store;
        }

        const auto err = nimble_port_init();
        if (err == ESP_OK) {
            state.ready = true;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t stop(const TickType_t wait = portMAX_DELAY) noexcept
    {
        const auto err = nimble_port_stop();
        if (const auto out = code(err); out != ESP_OK) {
            return out;
        }

        return join(wait);
    }

    [[nodiscard]] static esp_err_t deinit() noexcept
    {
        Gate guard(state.gate);
        if (!state.ready) {
            return ESP_OK;
        }
        if (active()) {
            return ESP_ERR_INVALID_STATE;
        }

        const auto err = nimble_port_deinit();
        if (err == ESP_OK) {
            state.ready = false;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t run() noexcept
    {
        if (!claim()) {
            return ESP_ERR_INVALID_STATE;
        }

        loop();
        return ESP_OK;
    }

    template <std::size_t StackBytes = 4096U,
              Core Cpu = Core::core0,
              UBaseType_t Priority = configMAX_PRIORITIES - 4U>
    [[nodiscard]] static esp_err_t host(const char* const name = "ble") noexcept
    {
        if (name == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }
        if (!ready()) {
            return ESP_ERR_INVALID_STATE;
        }

        if (!claim()) {
            return ESP_ERR_INVALID_STATE;
        }

        const auto handle = spawn(
            &entry<StackBytes, Cpu, Priority>,
            name,
            nullptr,
            Priority,
            Cpu,
            Slot<StackBytes, Cpu, Priority>::mem);
        if (handle == nullptr) {
            release();
            return ESP_ERR_NO_MEM;
        }

        return ESP_OK;
    }

    [[nodiscard]] static bool active() noexcept
    {
        return __atomic_load_n(&state.active, __ATOMIC_ACQUIRE) != 0U;
    }

    [[nodiscard]] static esp_err_t join(const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (!active()) {
            return ESP_OK;
        }
        if (xTaskGetCurrentTaskHandle() == task()) {
            return ESP_OK;
        }

        const auto start = xTaskGetTickCount();
        while (active()) {
            if (wait != portMAX_DELAY && static_cast<TickType_t>(xTaskGetTickCount() - start) >= wait) {
                return ESP_ERR_TIMEOUT;
            }
            vTaskDelay(1);
        }
        return ESP_OK;
    }

    [[nodiscard]] static bool ready() noexcept
    {
        Gate guard(state.gate);
        return state.ready;
    }

    [[nodiscard]] static struct ble_hs_cfg& cfg() noexcept
    {
        return ble_hs_cfg;
    }

    static void hook(
        ble_hs_reset_fn* const reset,
        ble_hs_sync_fn* const sync) noexcept
    {
        ble_hs_cfg.reset_cb = reset;
        ble_hs_cfg.sync_cb = sync;
    }

    static void gap() noexcept
    {
        ble_svc_gap_init();
    }

    [[nodiscard]] static esp_err_t name(const char* const value) noexcept
    {
        if (value == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return code(ble_svc_gap_device_name_set(value));
    }

    [[nodiscard]] static const char* name() noexcept
    {
        return ble_svc_gap_device_name();
    }

    [[nodiscard]] static esp_err_t appearance(const std::uint16_t value) noexcept
    {
        return code(ble_svc_gap_device_appearance_set(value));
    }

    [[nodiscard]] static esp_err_t ensure() noexcept
    {
        return code(ble_hs_util_ensure_addr(0));
    }

    [[nodiscard]] static esp_err_t own(
        std::uint8_t& type,
        const bool privacy = false) noexcept
    {
        const auto ready = ensure();
        if (ready != ESP_OK) {
            return ready;
        }

        return code(ble_hs_id_infer_auto(privacy ? 1 : 0, &type));
    }

    [[nodiscard]] static esp_err_t addr(
        const std::uint8_t type,
        Addr& out,
        bool* const nrpa = nullptr) noexcept
    {
        int flag{};
        const auto err = code(ble_hs_id_copy_addr(type, out.data(), &flag));
        if (err == ESP_OK && nrpa != nullptr) {
            *nrpa = flag != 0;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t fields(const ble_hs_adv_fields& value) noexcept
    {
        return code(ble_gap_adv_set_fields(&value));
    }

    [[nodiscard]] static esp_err_t reply(const ble_hs_adv_fields& value) noexcept
    {
        return code(ble_gap_adv_rsp_set_fields(&value));
    }

    [[nodiscard]] static esp_err_t adv(
        const std::uint8_t own_type,
        const ble_gap_adv_params& params,
        ble_gap_event_fn* const callback = nullptr,
        void* const arg = nullptr,
        const std::int32_t duration_ms = forever,
        const ble_addr_t* const direct = nullptr) noexcept
    {
        return code(ble_gap_adv_start(own_type, direct, duration_ms, &params, callback, arg));
    }

    [[nodiscard]] static esp_err_t adv_stop() noexcept
    {
        return code(ble_gap_adv_stop());
    }

    [[nodiscard]] static esp_err_t scan(
        const std::uint8_t own_type,
        const ble_gap_disc_params& params,
        ble_gap_event_fn* const callback,
        void* const arg = nullptr,
        const std::int32_t duration_ms = forever) noexcept
    {
        if (callback == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return code(ble_gap_disc(own_type, duration_ms, &params, callback, arg));
    }

    [[nodiscard]] static esp_err_t scan_stop() noexcept
    {
        return code(ble_gap_disc_cancel());
    }

    [[nodiscard]] static constexpr esp_err_t code(const int err) noexcept
    {
        switch (err) {
            case 0:
            case BLE_HS_EDONE:
                return ESP_OK;
            case BLE_HS_EAGAIN:
            case BLE_HS_EALREADY:
            case BLE_HS_ENOTCONN:
            case BLE_HS_EBUSY:
                return ESP_ERR_INVALID_STATE;
            case BLE_HS_EINVAL:
            case BLE_HS_EBADDATA:
                return ESP_ERR_INVALID_ARG;
            case BLE_HS_EMSGSIZE:
                return ESP_ERR_INVALID_SIZE;
            case BLE_HS_ENOENT:
            case BLE_HS_ENOADDR:
                return ESP_ERR_NOT_FOUND;
            case BLE_HS_ENOMEM:
            case BLE_HS_ENOMEM_EVT:
                return ESP_ERR_NO_MEM;
            case BLE_HS_ENOTSUP:
                return ESP_ERR_NOT_SUPPORTED;
            case BLE_HS_ETIMEOUT:
            case BLE_HS_ETIMEOUT_HCI:
                return ESP_ERR_TIMEOUT;
            case BLE_HS_ENOTSYNCED:
            case BLE_HS_EDISABLED:
                return ESP_ERR_INVALID_STATE;
            default:
                return ESP_FAIL;
        }
    }

private:
    template <std::size_t StackBytes, Core Cpu, UBaseType_t Priority>
    struct Slot {
        constinit static inline TaskMem<StackBytes> mem{};
    };

    template <std::size_t StackBytes, Core Cpu, UBaseType_t Priority>
    static void entry(void*) noexcept
    {
        loop();
        vTaskDelete(nullptr);
        __builtin_unreachable();
    }

    [[nodiscard]] static bool claim() noexcept
    {
        auto inactive = std::uint32_t{};
        return __atomic_compare_exchange_n(
            &state.active,
            &inactive,
            1U,
            false,
            __ATOMIC_ACQ_REL,
            __ATOMIC_ACQUIRE);
    }

    static void release() noexcept
    {
        __atomic_store_n(&state.active, 0U, __ATOMIC_RELEASE);
    }

    static void loop() noexcept
    {
        {
            Gate guard(state.gate);
            state.task = xTaskGetCurrentTaskHandle();
        }

        nimble_port_run();

        {
            Gate guard(state.gate);
            state.task = nullptr;
        }
        release();
    }

    [[nodiscard]] static TaskHandle_t task() noexcept
    {
        Gate guard(state.gate);
        return state.task;
    }

    struct State {
        std::uint32_t gate;
        std::uint32_t active;
        TaskHandle_t task;
        bool ready;
    };

    constinit static inline State state{
        0U,
        0U,
        nullptr,
        false,
    };
};

}  // namespace arc
