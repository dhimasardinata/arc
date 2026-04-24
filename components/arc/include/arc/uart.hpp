#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/claim.hpp"
#include "arc/init.hpp"
#include "arc/result.hpp"

namespace arc {

template <typename T>
concept UartBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <uart_port_t Port,
          int Tx,
          int Rx,
          int Rts = UART_PIN_NO_CHANGE,
          int Cts = UART_PIN_NO_CHANGE,
          int Baud = 115200,
          int RxBuf = 1024,
          int TxBuf = 0,
          int Queue = 0,
          int Intr = 0,
          uart_word_length_t Bits = UART_DATA_8_BITS,
          uart_parity_t Parity = UART_PARITY_DISABLE,
          uart_stop_bits_t Stop = UART_STOP_BITS_1,
          uart_hw_flowcontrol_t Flow = UART_HW_FLOWCTRL_DISABLE,
          uart_sclk_t Clock = UART_SCLK_DEFAULT>
struct Uart {
    static_assert(Port >= 0 && Port < SOC_UART_NUM, "invalid UART port");
    static_assert((Tx >= 0 && Tx < SOC_GPIO_PIN_COUNT) || Tx == UART_PIN_NO_CHANGE, "invalid UART TX pin");
    static_assert((Rx >= 0 && Rx < SOC_GPIO_PIN_COUNT) || Rx == UART_PIN_NO_CHANGE, "invalid UART RX pin");
    static_assert((Rts >= 0 && Rts < SOC_GPIO_PIN_COUNT) || Rts == UART_PIN_NO_CHANGE, "invalid UART RTS pin");
    static_assert((Cts >= 0 && Cts < SOC_GPIO_PIN_COUNT) || Cts == UART_PIN_NO_CHANGE, "invalid UART CTS pin");
    static_assert(Baud > 0, "UART baud must be non-zero");
    static_assert(RxBuf > 0, "UART RX buffer must be non-zero");
    static_assert(TxBuf >= 0, "UART TX buffer cannot be negative");
    static_assert(Queue >= 0, "UART queue depth cannot be negative");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        uart_config_t cfg{};
        cfg.baud_rate = Baud;
        cfg.data_bits = Bits;
        cfg.parity = Parity;
        cfg.stop_bits = Stop;
        cfg.flow_ctrl = Flow;
        cfg.rx_flow_ctrl_thresh = 122;
        cfg.source_clk = Clock;
        cfg.flags.allow_pd = false;
        cfg.flags.backup_before_sleep = false;

        err = uart_param_config(Port, &cfg);
        if (err != ESP_OK) {
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        err = uart_set_pin(Port, Tx, Rx, Rts, Cts);
        if (err != ESP_OK) {
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        err = uart_driver_install(Port, RxBuf, TxBuf, Queue, nullptr, Intr);
        if (err == ESP_ERR_INVALID_STATE) {
            Init::pass(state.init);
            return ESP_OK;
        }
        if (err != ESP_OK) {
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static esp_err_t write(
        const void* const data,
        const std::size_t bytes,
        std::size_t* const wrote) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        const auto out = uart_write_bytes(Port, data, bytes);
        if (out < 0) {
            return ESP_FAIL;
        }
        if (wrote != nullptr) {
            *wrote = static_cast<std::size_t>(out);
        }
        return ESP_OK;
    }

    [[nodiscard]] static Result<std::size_t> write(
        const void* const data,
        const std::size_t bytes) noexcept
    {
        std::size_t wrote{};
        const auto err = write(data, bytes, &wrote);
        if (err != ESP_OK) {
            return fail(err);
        }
        return wrote;
    }

    template <typename T, std::size_t Extent>
        requires UartBytes<T>
    [[nodiscard]] static Result<std::size_t> write(const std::span<T, Extent> data) noexcept
    {
        return write(data.data(), data.size_bytes());
    }

    [[nodiscard]] static esp_err_t read(
        void* const data,
        const std::size_t bytes,
        std::size_t* const got,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        const auto in = uart_read_bytes(Port, data, bytes, pdMS_TO_TICKS(timeout_ms));
        if (in < 0) {
            return ESP_FAIL;
        }
        if (got != nullptr) {
            *got = static_cast<std::size_t>(in);
        }
        return ESP_OK;
    }

    [[nodiscard]] static Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        std::size_t got{};
        const auto err = read(data, bytes, &got, timeout_ms);
        if (err != ESP_OK) {
            return fail(err);
        }
        return got;
    }

    template <typename T, std::size_t Extent>
        requires(UartBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static Result<std::size_t> read(
        const std::span<T, Extent> data,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        return read(data.data(), data.size_bytes(), timeout_ms);
    }

    [[nodiscard]] static esp_err_t wait(const std::uint32_t timeout_ms = 1000U) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return uart_wait_tx_done(Port, pdMS_TO_TICKS(timeout_ms));
    }

    [[nodiscard]] static esp_err_t flush() noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return uart_flush_input(Port);
    }

    [[nodiscard]] static esp_err_t available(std::size_t& bytes) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return uart_get_buffered_data_len(Port, &bytes);
    }

    [[nodiscard]] static esp_err_t baud(const std::uint32_t value) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return uart_set_baudrate(Port, value);
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = uart_driver_delete(Port);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static constexpr uart_port_t port() noexcept
    {
        return Port;
    }

    [[nodiscard]] static constexpr int tx() noexcept
    {
        return Tx;
    }

    [[nodiscard]] static constexpr int rx() noexcept
    {
        return Rx;
    }

private:
    using Resource = Claim<ClaimKind::uart,
                           static_cast<int>(Port),
                           claim_token<Port, Tx, Rx, Rts, Cts, Baud, RxBuf, TxBuf, Queue, Intr, Bits, Parity, Stop, Flow, Clock>()>;

    struct State {
        std::uint32_t init;
    };

    constinit static inline State state{0U};
};

}  // namespace arc
