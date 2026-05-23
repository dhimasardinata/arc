#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

enum spi_host_device_t {
    SPI1_HOST = 0,
    SPI2_HOST = 1,
    SPI3_HOST = 2,
};

using spi_dma_chan_t = int;
using spi_clock_source_t = int;
using spi_device_handle_t = void*;

constexpr spi_dma_chan_t SPI_DMA_CH_AUTO = -1;
constexpr spi_clock_source_t SPI_CLK_SRC_DEFAULT = 0;
constexpr std::uint32_t SPI_MASTER_FREQ_20M = 20'000'000U;
constexpr std::uint32_t SPI_TRANS_MODE_DIO = 1U << 0U;
constexpr std::uint32_t SPI_TRANS_MODE_QIO = 1U << 1U;
constexpr std::uint32_t SPI_TRANS_MODE_OCT = 1U << 2U;
constexpr int SPI_SAMPLING_POINT_PHASE_0 = 0;

struct spi_bus_config_t {
    int sclk_io_num{};
    int mosi_io_num{};
    int miso_io_num{};
    int quadwp_io_num{};
    int quadhd_io_num{};
    int data4_io_num{};
    int data5_io_num{};
    int data6_io_num{};
    int data7_io_num{};
    int max_transfer_sz{};
    std::uint32_t flags{};
    int isr_cpu_id{};
    int intr_flags{};
    bool data_io_default_level{};
};

struct spi_device_interface_config_t {
    int command_bits{};
    int address_bits{};
    int dummy_bits{};
    int mode{};
    spi_clock_source_t clock_source{};
    int duty_cycle_pos{};
    int cs_ena_pretrans{};
    int cs_ena_posttrans{};
    int clock_speed_hz{};
    int input_delay_ns{};
    int sample_point{};
    int spics_io_num{};
    std::uint32_t flags{};
    int queue_size{};
    void (*pre_cb)(void*){};
    void (*post_cb)(void*){};
};

struct spi_transaction_t {
    std::uint32_t flags{};
    std::size_t length{};
    std::size_t rxlength{};
    int override_freq_hz{};
    void* user{};
    const void* tx_buffer{};
    void* rx_buffer{};
};

inline std::uint32_t arc_host_spi_transmit_calls{};
inline std::uint32_t arc_host_spi_poll_calls{};
inline std::uint32_t arc_host_spi_queue_calls{};

inline esp_err_t spi_bus_initialize(spi_host_device_t, const spi_bus_config_t*, spi_dma_chan_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_bus_free(spi_host_device_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_bus_add_device(
    spi_host_device_t,
    const spi_device_interface_config_t*,
    spi_device_handle_t* handle) noexcept
{
    if (handle != nullptr) {
        *handle = reinterpret_cast<spi_device_handle_t>(1);
    }
    return ESP_OK;
}

inline esp_err_t spi_bus_remove_device(spi_device_handle_t) noexcept
{
    return ESP_OK;
}

inline esp_err_t spi_device_acquire_bus(spi_device_handle_t, TickType_t) noexcept
{
    return ESP_OK;
}

inline void spi_device_release_bus(spi_device_handle_t) noexcept {}

inline esp_err_t spi_device_transmit(spi_device_handle_t, spi_transaction_t*) noexcept
{
    ++arc_host_spi_transmit_calls;
    return ESP_OK;
}

inline esp_err_t spi_device_polling_transmit(spi_device_handle_t, spi_transaction_t*) noexcept
{
    ++arc_host_spi_poll_calls;
    return ESP_OK;
}

inline esp_err_t spi_device_queue_trans(spi_device_handle_t, spi_transaction_t*, TickType_t) noexcept
{
    ++arc_host_spi_queue_calls;
    return ESP_OK;
}

inline esp_err_t spi_device_get_trans_result(spi_device_handle_t, spi_transaction_t**, TickType_t) noexcept
{
    return ESP_ERR_TIMEOUT;
}
