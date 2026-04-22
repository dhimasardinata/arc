#pragma once

#include <cstdint>
#include <utility>

#include "esp_check.h"
#include "esp_err.h"
#include "esp_pm.h"

#include "arc/init.hpp"

namespace arc {

template <esp_pm_lock_type_t Type, int Arg = 0>
struct Pm {
    [[nodiscard]] static esp_err_t init(const char* const name = "arc") noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        const auto err = esp_pm_lock_create(Type, Arg, name, &state.handle);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            state.handle = nullptr;
            Init::fail(state.init);
        }
        return err;
    }

    static void boot(const char* const name = "arc")
    {
        ESP_ERROR_CHECK(init(name));
    }

    [[nodiscard]] static esp_err_t acquire(const char* const name = "arc") noexcept
    {
        const auto err = init(name);
        if (err != ESP_OK) {
            return err;
        }
        return esp_pm_lock_acquire(state.handle);
    }

    [[nodiscard]] static esp_err_t release() noexcept
    {
        if (state.handle == nullptr) {
            return ESP_ERR_INVALID_STATE;
        }
        return esp_pm_lock_release(state.handle);
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = esp_pm_lock_delete(state.handle);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.handle = nullptr;
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static esp_pm_lock_handle_t native(const char* const name = "arc") noexcept
    {
        return init(name) == ESP_OK ? state.handle : nullptr;
    }

    struct Hold {
        explicit Hold(const char* const name = "arc") noexcept
            : owned_(Pm::acquire(name) == ESP_OK)
        {
        }

        Hold(const Hold&) = delete;
        Hold& operator=(const Hold&) = delete;

        Hold(Hold&& other) noexcept
            : owned_(std::exchange(other.owned_, false))
        {
        }

        Hold& operator=(Hold&& other) noexcept
        {
            if (this != &other) {
                reset();
                owned_ = std::exchange(other.owned_, false);
            }
            return *this;
        }

        ~Hold()
        {
            reset();
        }

        [[nodiscard]] bool active() const noexcept
        {
            return owned_;
        }

        [[nodiscard]] esp_err_t reset() noexcept
        {
            if (!owned_) {
                return ESP_OK;
            }
            owned_ = false;
            return Pm::release();
        }

    private:
        bool owned_;
    };

    [[nodiscard]] static Hold hold(const char* const name = "arc") noexcept
    {
        return Hold{name};
    }

private:
    struct State {
        esp_pm_lock_handle_t handle{};
        std::uint32_t init{};
    };

    constinit static inline State state{};
};

using CpuLock = Pm<ESP_PM_CPU_FREQ_MAX>;
using ApbLock = Pm<ESP_PM_APB_FREQ_MAX>;
using SleepLock = Pm<ESP_PM_NO_LIGHT_SLEEP>;

}  // namespace arc
