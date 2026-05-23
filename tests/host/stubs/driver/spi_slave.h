#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

constexpr std::uint32_t SPICOMMON_BUSFLAG_SLAVE = 1U << 8U;
constexpr std::uint32_t SPI_SLAVE_NO_RETURN_RESULT = 1U << 9U;

struct spi_slave_transaction_t {
    std::uint32_t flags{};
    std::size_t length{};
    const void* tx_buffer{};
    void* rx_buffer{};
    void* user{};
};

struct spi_slave_interface_config_t {
    int spics_io_num{};
    std::uint32_t flags{};
    int queue_size{};
    int mode{};
    void (*post_setup_cb)(void*){};
    void (*post_trans_cb)(void*){};
};

inline std::uint32_t arc_host_spi_slave_transmit_calls{};
inline std::uint32_t arc_host_spi_slave_queue_calls{};

inline esp_err_t spi_slave_initialize(
    spi_host_device_t,
    const spi_bus_config_t*,
    const spi_slave_interface_config_t*,
    spi_dma_chan_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_slave_enable(spi_host_device_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_slave_disable(spi_host_device_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_slave_free(spi_host_device_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_slave_transmit(spi_host_device_t, spi_slave_transaction_t*, TickType_t) noexcept
{
    ++arc_host_spi_slave_transmit_calls;
    return ESP_OK;
}

inline esp_err_t spi_slave_queue_trans(spi_host_device_t, spi_slave_transaction_t*, TickType_t) noexcept
{
    ++arc_host_spi_slave_queue_calls;
    return ESP_OK;
}

inline esp_err_t spi_slave_get_trans_result(spi_host_device_t, spi_slave_transaction_t**, TickType_t) noexcept
{
    return ESP_ERR_TIMEOUT;
}
