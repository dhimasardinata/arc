#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <typename T>
concept I2cBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <int Port,
          int Sda,
          int Scl,
          bool Pullup = true,
          std::uint8_t Glitch = 7,
          i2c_clock_source_t Clock = I2C_CLK_SRC_DEFAULT,
          int Intr = 0,
          std::size_t Queue = 0>
struct I2cBus {
    static_assert(Port >= 0 && Port < SOC_I2C_NUM, "invalid I2C port");
    static_assert(Sda >= 0 && Sda < SOC_GPIO_PIN_COUNT, "invalid I2C SDA pin");
    static_assert(Scl >= 0 && Scl < SOC_GPIO_PIN_COUNT, "invalid I2C SCL pin");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (state.bus != nullptr) {
            return ESP_OK;
        }

        i2c_master_bus_config_t cfg{};
        cfg.i2c_port = static_cast<decltype(cfg.i2c_port)>(Port);
        cfg.sda_io_num = static_cast<gpio_num_t>(Sda);
        cfg.scl_io_num = static_cast<gpio_num_t>(Scl);
        cfg.clk_source = Clock;
        cfg.glitch_ignore_cnt = Glitch;
        cfg.intr_priority = Intr;
        cfg.trans_queue_depth = Queue;
        cfg.flags.enable_internal_pullup = Pullup;
        cfg.flags.allow_pd = false;

        const auto err = i2c_new_master_bus(&cfg, &state.bus);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        if (err == ESP_ERR_INVALID_STATE) {
            return i2c_master_get_bus_handle(static_cast<decltype(cfg.i2c_port)>(Port), &state.bus);
        }
        return err;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static esp_err_t probe(
        const std::uint16_t addr,
        const int timeout_ms = 50) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return i2c_master_probe(state.bus, addr, timeout_ms);
    }

    [[nodiscard]] static i2c_master_bus_handle_t native() noexcept
    {
        boot();
        return state.bus;
    }

    [[nodiscard]] static constexpr int port() noexcept
    {
        return Port;
    }

    [[nodiscard]] static constexpr int sda() noexcept
    {
        return Sda;
    }

    [[nodiscard]] static constexpr int scl() noexcept
    {
        return Scl;
    }

private:
    struct State {
        i2c_master_bus_handle_t bus;
    };

    constinit static inline State state{nullptr};
};

template <typename Bus,
          std::uint16_t Addr,
          std::uint32_t Hz = 400'000,
          i2c_addr_bit_len_t AddrLen = I2C_ADDR_BIT_LEN_7,
          std::uint32_t SclWaitUs = 0,
          bool AckCheck = true>
struct I2c {
    static_assert(Addr <= 0x3FFU, "invalid I2C device address");
    static_assert(Hz != 0U, "I2C clock must be non-zero");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (state.dev != nullptr) {
            return ESP_OK;
        }

        auto err = Bus::init();
        if (err != ESP_OK) {
            return err;
        }

        i2c_device_config_t cfg{};
        cfg.dev_addr_length = AddrLen;
        cfg.device_address = Addr;
        cfg.scl_speed_hz = Hz;
        cfg.scl_wait_us = SclWaitUs;
        cfg.flags.disable_ack_check = !AckCheck;

        return i2c_master_bus_add_device(Bus::native(), &cfg, &state.dev);
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms = 1000) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return i2c_master_transmit(
            state.dev,
            static_cast<const std::uint8_t*>(data),
            bytes,
            timeout_ms);
    }

    [[nodiscard]] static esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const int timeout_ms = 1000) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return i2c_master_receive(
            state.dev,
            static_cast<std::uint8_t*>(data),
            bytes,
            timeout_ms);
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms = 1000) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return i2c_master_transmit_receive(
            state.dev,
            static_cast<const std::uint8_t*>(tx),
            tx_bytes,
            static_cast<std::uint8_t*>(rx),
            rx_bytes,
            timeout_ms);
    }

    template <typename T>
        requires I2cBytes<T>
    [[nodiscard]] static esp_err_t send(
        const std::span<T> data,
        const int timeout_ms = 1000) noexcept
    {
        return send(data.data(), data.size_bytes(), timeout_ms);
    }

    template <typename T>
        requires(I2cBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static esp_err_t recv(
        const std::span<T> data,
        const int timeout_ms = 1000) noexcept
    {
        return recv(data.data(), data.size_bytes(), timeout_ms);
    }

    template <typename Tx, typename Rx>
        requires(I2cBytes<Tx> && I2cBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t xfer(
        const std::span<Tx> tx,
        const std::span<Rx> rx,
        const int timeout_ms = 1000) noexcept
    {
        return xfer(tx.data(), tx.size_bytes(), rx.data(), rx.size_bytes(), timeout_ms);
    }

    [[nodiscard]] static i2c_master_dev_handle_t native() noexcept
    {
        boot();
        return state.dev;
    }

    [[nodiscard]] static constexpr std::uint16_t address() noexcept
    {
        return Addr;
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

private:
    struct State {
        i2c_master_dev_handle_t dev;
    };

    constinit static inline State state{nullptr};
};

}  // namespace arc
