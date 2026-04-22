#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "driver/usb_serial_jtag.h"
#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"
#include "arc/result.hpp"

namespace arc {

template <typename T>
concept UsbBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <std::uint32_t Tx = 256U, std::uint32_t Rx = 256U>
struct Usb {
    static_assert(SOC_USB_SERIAL_JTAG_SUPPORTED, "arc::Usb requires ESP32-S3 USB Serial/JTAG");
    static_assert(Tx > 0U, "USB TX buffer must be non-zero");
    static_assert(Rx > 0U, "USB RX buffer must be non-zero");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        if (usb_serial_jtag_is_driver_installed()) {
            Init::pass(state.init);
            return ESP_OK;
        }

        usb_serial_jtag_driver_config_t cfg{};
        cfg.tx_buffer_size = Tx;
        cfg.rx_buffer_size = Rx;
        const auto err = usb_serial_jtag_driver_install(&cfg);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            Init::fail(state.init);
        }
        return err;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static Result<std::size_t> write(
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        if (data == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        boot();
        const auto out = usb_serial_jtag_write_bytes(data, bytes, ticks(timeout_ms));
        if (out < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::size_t>(out);
    }

    template <typename T, std::size_t Extent>
        requires UsbBytes<T>
    [[nodiscard]] static Result<std::size_t> write(
        const std::span<T, Extent> data,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        return write(data.data(), data.size_bytes(), timeout_ms);
    }

    [[nodiscard]] static Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        if (data == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        if (bytes > std::numeric_limits<std::uint32_t>::max()) {
            return fail(ESP_ERR_INVALID_SIZE);
        }

        boot();
        const auto in = usb_serial_jtag_read_bytes(
            data,
            static_cast<std::uint32_t>(bytes),
            ticks(timeout_ms));
        if (in < 0) {
            return fail(ESP_FAIL);
        }
        return static_cast<std::size_t>(in);
    }

    template <typename T, std::size_t Extent>
        requires(UsbBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static Result<std::size_t> read(
        const std::span<T, Extent> data,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        return read(data.data(), data.size_bytes(), timeout_ms);
    }

    [[nodiscard]] static esp_err_t wait(const std::uint32_t timeout_ms = 1000U) noexcept
    {
        boot();
        return usb_serial_jtag_wait_tx_done(ticks(timeout_ms));
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = usb_serial_jtag_driver_uninstall();
        if (err == ESP_OK) {
            Init::fail(state.init);
        } else {
            Init::pass(state.init);
        }
        return err;
    }

    [[nodiscard]] static bool connected() noexcept
    {
        return usb_serial_jtag_is_connected();
    }

    [[nodiscard]] static bool installed() noexcept
    {
        return usb_serial_jtag_is_driver_installed();
    }

private:
    struct State {
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static constexpr std::uint32_t ticks(const std::uint32_t ms) noexcept
    {
        return static_cast<std::uint32_t>(pdMS_TO_TICKS(ms));
    }
};

}  // namespace arc
