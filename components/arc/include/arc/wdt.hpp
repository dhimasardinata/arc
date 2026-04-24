#pragma once

#include <cstdint>
#include <utility>

#include "esp_err.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "arc/result.hpp"

namespace arc {

struct Wdt {
    static_assert(SOC_WDT_SUPPORTED, "watchdog timer is not supported on this target");

    struct User {
        esp_task_wdt_user_handle_t handle{};

        constexpr User() noexcept = default;

        explicit constexpr User(const esp_task_wdt_user_handle_t raw) noexcept
            : handle(raw)
        {
        }

        User(const User&) = delete;
        User& operator=(const User&) = delete;

        User(User&& other) noexcept
            : handle(std::exchange(other.handle, nullptr))
        {
        }

        User& operator=(User&& other) noexcept
        {
            if (this != &other) {
                static_cast<void>(off());
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        ~User()
        {
            static_cast<void>(off());
        }

        [[nodiscard]] esp_err_t feed() const noexcept
        {
            return handle == nullptr ? ESP_ERR_INVALID_STATE : esp_task_wdt_reset_user(handle);
        }

        [[nodiscard]] esp_err_t off() noexcept
        {
            if (handle == nullptr) {
                return ESP_OK;
            }

            const auto raw = std::exchange(handle, nullptr);
            return esp_task_wdt_delete_user(raw);
        }

        [[nodiscard]] esp_task_wdt_user_handle_t native() const noexcept
        {
            return handle;
        }

        explicit constexpr operator bool() const noexcept
        {
            return handle != nullptr;
        }
    };

    [[nodiscard]] static esp_err_t init(
        const std::uint32_t timeout_ms,
        const std::uint32_t idle_cores = 0U,
        const bool panic = true) noexcept
    {
        const esp_task_wdt_config_t config{
            .timeout_ms = timeout_ms,
            .idle_core_mask = idle_cores,
            .trigger_panic = panic,
        };
        const auto err = esp_task_wdt_init(&config);
        return err == ESP_ERR_INVALID_STATE ? ESP_OK : err;
    }

    template <std::uint32_t TimeoutMs, std::uint32_t IdleCores = 0U, bool Panic = true>
    [[nodiscard]] static esp_err_t init() noexcept
    {
        static_assert(TimeoutMs != 0U, "watchdog timeout must be non-zero");
        return init(TimeoutMs, IdleCores, Panic);
    }

    [[nodiscard]] static esp_err_t tune(
        const std::uint32_t timeout_ms,
        const std::uint32_t idle_cores = 0U,
        const bool panic = true) noexcept
    {
        const esp_task_wdt_config_t config{
            .timeout_ms = timeout_ms,
            .idle_core_mask = idle_cores,
            .trigger_panic = panic,
        };
        return esp_task_wdt_reconfigure(&config);
    }

    template <std::uint32_t TimeoutMs, std::uint32_t IdleCores = 0U, bool Panic = true>
    [[nodiscard]] static esp_err_t tune() noexcept
    {
        static_assert(TimeoutMs != 0U, "watchdog timeout must be non-zero");
        return tune(TimeoutMs, IdleCores, Panic);
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        return esp_task_wdt_deinit();
    }

    [[nodiscard]] static esp_err_t watch(TaskHandle_t task = nullptr) noexcept
    {
        return esp_task_wdt_add(task);
    }

    [[nodiscard]] static esp_err_t unwatch(TaskHandle_t task = nullptr) noexcept
    {
        return esp_task_wdt_delete(task);
    }

    [[nodiscard]] static esp_err_t feed() noexcept
    {
        return esp_task_wdt_reset();
    }

    [[nodiscard]] static esp_err_t check(TaskHandle_t task = nullptr) noexcept
    {
        return esp_task_wdt_status(task);
    }

    [[nodiscard]] static Result<User> user(const char* const name) noexcept
    {
        esp_task_wdt_user_handle_t raw{};
        const auto err = esp_task_wdt_add_user(name, &raw);
        if (err != ESP_OK) {
            return fail(err);
        }
        return User{raw};
    }
};

}  // namespace arc
