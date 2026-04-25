#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "driver/i2c_slave.h"
#include "esp_check.h"
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/claim.hpp"
#include "arc/init.hpp"

namespace arc {

template <typename T>
concept I2cSlaveBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <int Port,
          int Sda,
          int Scl,
          std::uint16_t Addr,
          std::uint32_t TxBytes = 256,
          std::uint32_t RxBytes = 256,
          bool Pullup = true,
          i2c_addr_bit_len_t AddrLen = I2C_ADDR_BIT_LEN_7,
          i2c_clock_source_t Clock = I2C_CLK_SRC_DEFAULT,
          int Intr = 0,
          bool AllowPd = false,
          bool Broadcast = false>
struct I2cSlave {
    static_assert(Port >= 0 && Port < SOC_I2C_NUM, "invalid I2C slave port");
    static_assert(Sda >= 0 && Sda < SOC_GPIO_PIN_COUNT, "invalid I2C slave SDA pin");
    static_assert(Scl >= 0 && Scl < SOC_GPIO_PIN_COUNT, "invalid I2C slave SCL pin");
    static_assert(TxBytes > 0U, "I2C slave TX buffer must be non-zero");
    static_assert(RxBytes > 0U, "I2C slave RX buffer must be non-zero");
    static_assert(AddrLen == I2C_ADDR_BIT_LEN_7 || AddrLen == I2C_ADDR_BIT_LEN_10,
                  "I2C slave address length must be 7 or 10 bits");
    static_assert((AddrLen == I2C_ADDR_BIT_LEN_7 && Addr <= 0x7FU) ||
                      (AddrLen == I2C_ADDR_BIT_LEN_10 && Addr <= 0x3FFU),
                  "invalid I2C slave address");
    static_assert(!(Broadcast && AddrLen == I2C_ADDR_BIT_LEN_10),
                  "I2C slave broadcast requires a 7-bit address");
#if defined(SOC_I2C_SLAVE_SUPPORT_BROADCAST)
    static_assert(!Broadcast || SOC_I2C_SLAVE_SUPPORT_BROADCAST, "I2C slave broadcast is not supported");
#else
    static_assert(!Broadcast, "I2C slave broadcast is not supported");
#endif

    using Handle = i2c_slave_dev_handle_t;
    using Callbacks = i2c_slave_event_callbacks_t;
    using Request = i2c_slave_request_event_data_t;
    using Receive = i2c_slave_rx_done_event_data_t;

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

        i2c_slave_config_t cfg{};
        cfg.i2c_port = static_cast<i2c_port_num_t>(Port);
        cfg.sda_io_num = static_cast<gpio_num_t>(Sda);
        cfg.scl_io_num = static_cast<gpio_num_t>(Scl);
        cfg.clk_source = Clock;
        cfg.send_buf_depth = TxBytes;
        cfg.receive_buf_depth = RxBytes;
        cfg.slave_addr = Addr;
        cfg.addr_bit_len = AddrLen;
        cfg.intr_priority = Intr;
        cfg.flags.allow_pd = AllowPd;
        cfg.flags.enable_internal_pullup = Pullup;
#if defined(SOC_I2C_SLAVE_SUPPORT_BROADCAST) && SOC_I2C_SLAVE_SUPPORT_BROADCAST
        cfg.flags.broadcast_en = Broadcast;
#endif

        err = i2c_new_slave_device(&cfg, &state.dev);
        if (err != ESP_OK) {
            state.dev = nullptr;
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

    [[nodiscard]] static esp_err_t listen(
        const i2c_slave_request_callback_t request,
        const i2c_slave_received_callback_t receive,
        void* const user = nullptr) noexcept
    {
        const Callbacks callbacks{
            .on_request = request,
            .on_receive = receive,
        };
        return listen(callbacks, user);
    }

    [[nodiscard]] static esp_err_t listen(
        const Callbacks& callbacks,
        void* const user = nullptr) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return i2c_slave_register_event_callbacks(state.dev, &callbacks, user);
    }

    [[nodiscard]] static esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        std::size_t& written,
        const int timeout_ms = 1000) noexcept
    {
        written = 0U;
        if (data == nullptr || bytes == 0U || bytes > 0xFFFF'FFFFULL) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        std::uint32_t out{};
        const auto err = i2c_slave_write(
            state.dev,
            static_cast<const std::uint8_t*>(data),
            static_cast<std::uint32_t>(bytes),
            &out,
            timeout_ms);
        written = out;
        return err;
    }

    [[nodiscard]] static esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms = 1000) noexcept
    {
        std::size_t written{};
        return send(data, bytes, written, timeout_ms);
    }

    template <typename T, std::size_t Extent>
        requires I2cSlaveBytes<T>
    [[nodiscard]] static esp_err_t send(
        const std::span<T, Extent> data,
        std::size_t& written,
        const int timeout_ms = 1000) noexcept
    {
        return send(data.data(), data.size_bytes(), written, timeout_ms);
    }

    template <typename T, std::size_t Extent>
        requires I2cSlaveBytes<T>
    [[nodiscard]] static esp_err_t send(
        const std::span<T, Extent> data,
        const int timeout_ms = 1000) noexcept
    {
        return send(data.data(), data.size_bytes(), timeout_ms);
    }

    [[nodiscard]] static esp_err_t reset() noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return i2c_slave_reset_tx_fifo(state.dev);
    }

    [[nodiscard]] static Handle native() noexcept
    {
        boot();
        return state.dev;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = i2c_del_slave_device(state.dev);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.dev = nullptr;
        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
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

    [[nodiscard]] static constexpr std::uint16_t address() noexcept
    {
        return Addr;
    }

    [[nodiscard]] static constexpr std::uint32_t tx_bytes() noexcept
    {
        return TxBytes;
    }

    [[nodiscard]] static constexpr std::uint32_t rx_bytes() noexcept
    {
        return RxBytes;
    }

private:
    using Resource = Claim<ClaimKind::i2c_bus,
                           Port,
                           claim_token<Port,
                                       Sda,
                                       Scl,
                                       Addr,
                                       TxBytes,
                                       RxBytes,
                                       Pullup,
                                       AddrLen,
                                       Clock,
                                       Intr,
                                       AllowPd,
                                       Broadcast>()>;

    struct State {
        Handle dev;
        std::uint32_t init;
    };

    constinit static inline State state{nullptr, 0U};
};

}  // namespace arc
