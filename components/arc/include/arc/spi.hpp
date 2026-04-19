#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_intr_types.h"
#include "freertos/FreeRTOS.h"
#include "soc/soc_caps.h"

namespace arc {

template <spi_host_device_t Host,
          int Sclk,
          int Mosi,
          int Miso = -1,
          int MaxBytes = 4096,
          spi_dma_chan_t Dma = SPI_DMA_CH_AUTO,
          std::uint32_t Flags = 0U,
          esp_intr_cpu_affinity_t IsrCpu = ESP_INTR_CPU_AFFINITY_0,
          int IntrFlags = 0>
struct SpiBus {
    static_assert(Sclk >= 0 && Sclk < SOC_GPIO_PIN_COUNT, "invalid SPI SCLK pin");
    static_assert((Mosi >= 0 && Mosi < SOC_GPIO_PIN_COUNT) || Mosi == -1, "invalid SPI MOSI pin");
    static_assert((Miso >= 0 && Miso < SOC_GPIO_PIN_COUNT) || Miso == -1, "invalid SPI MISO pin");
    static_assert(Mosi != -1 || Miso != -1, "SPI bus needs MOSI, MISO, or both");
    static_assert(MaxBytes > 0, "SPI max transfer must be non-zero");

    static void boot()
    {
        if (state.ready) {
            return;
        }

        spi_bus_config_t cfg{};
        cfg.sclk_io_num = Sclk;
        cfg.mosi_io_num = Mosi;
        cfg.miso_io_num = Miso;
        cfg.quadwp_io_num = -1;
        cfg.quadhd_io_num = -1;
        cfg.data4_io_num = -1;
        cfg.data5_io_num = -1;
        cfg.data6_io_num = -1;
        cfg.data7_io_num = -1;
        cfg.max_transfer_sz = MaxBytes;
        cfg.flags = Flags;
        cfg.isr_cpu_id = IsrCpu;
        cfg.intr_flags = IntrFlags;
        cfg.data_io_default_level = false;
        ESP_ERROR_CHECK(spi_bus_initialize(Host, &cfg, Dma));
        state.ready = true;
    }

    [[nodiscard]] static constexpr spi_host_device_t host() noexcept
    {
        return Host;
    }

    [[nodiscard]] static constexpr int sclk() noexcept
    {
        return Sclk;
    }

    [[nodiscard]] static constexpr int mosi() noexcept
    {
        return Mosi;
    }

    [[nodiscard]] static constexpr int miso() noexcept
    {
        return Miso;
    }

    [[nodiscard]] static constexpr int max_bytes() noexcept
    {
        return MaxBytes;
    }

private:
    struct State {
        bool ready{};
    };

    constinit static inline State state{};
};

template <typename Bus,
          int Cs = -1,
          std::uint32_t Hz = SPI_MASTER_FREQ_20M,
          std::uint8_t Mode = 0,
          int Queue = 4,
          std::uint32_t Flags = 0U,
          spi_clock_source_t Clock = SPI_CLK_SRC_DEFAULT,
          std::uint16_t DutyCyclePos = 128,
          int InputDelayNs = 0>
struct Spi {
    static_assert((Cs >= 0 && Cs < SOC_GPIO_PIN_COUNT) || Cs == -1, "invalid SPI CS pin");
    static_assert(Hz != 0U, "SPI frequency must be non-zero");
    static_assert(Mode < 4U, "SPI mode must be in [0, 3]");
    static_assert(Queue > 0, "SPI queue depth must be non-zero");

    static void boot()
    {
        if (state.dev != nullptr) {
            return;
        }

        Bus::boot();

        spi_device_interface_config_t cfg{};
        cfg.command_bits = 0;
        cfg.address_bits = 0;
        cfg.dummy_bits = 0;
        cfg.mode = Mode;
        cfg.clock_source = Clock;
        cfg.duty_cycle_pos = DutyCyclePos;
        cfg.cs_ena_pretrans = 0;
        cfg.cs_ena_posttrans = 0;
        cfg.clock_speed_hz = static_cast<int>(Hz);
        cfg.input_delay_ns = InputDelayNs;
        cfg.sample_point = SPI_SAMPLING_POINT_PHASE_0;
        cfg.spics_io_num = Cs;
        cfg.flags = Flags;
        cfg.queue_size = Queue;
        cfg.pre_cb = nullptr;
        cfg.post_cb = nullptr;
        ESP_ERROR_CHECK(spi_bus_add_device(Bus::host(), &cfg, &state.dev));
    }

    static void acquire(const TickType_t wait = portMAX_DELAY)
    {
        boot();
        ESP_ERROR_CHECK(spi_device_acquire_bus(state.dev, wait));
        state.held = true;
    }

    static void release() noexcept
    {
        if (state.dev == nullptr || !state.held) {
            return;
        }

        spi_device_release_bus(state.dev);
        state.held = false;
    }

    [[nodiscard]] static spi_transaction_t job(
        const void* const tx,
        void* const rx,
        const std::size_t bytes) noexcept
    {
        spi_transaction_t trans{};
        trans.length = bytes * 8U;
        trans.rxlength = rx == nullptr ? 0U : bytes * 8U;
        trans.override_freq_hz = 0U;
        trans.user = nullptr;
        trans.tx_buffer = tx;
        trans.rx_buffer = rx;
        return trans;
    }

    [[nodiscard]] static esp_err_t send(const void* const tx, const std::size_t bytes) noexcept
    {
        boot();
        auto trans = job(tx, nullptr, bytes);
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t recv(void* const rx, const std::size_t bytes) noexcept
    {
        boot();
        auto trans = job(nullptr, rx, bytes);
        trans.rxlength = bytes * 8U;
        trans.length = bytes * 8U;
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes) noexcept
    {
        boot();
        auto trans = job(tx, rx, bytes);
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t poll(
        const void* const tx,
        void* const rx,
        const std::size_t bytes) noexcept
    {
        boot();
        auto trans = job(tx, rx, bytes);
        return spi_device_polling_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t queue(
        spi_transaction_t& trans,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        boot();
        return spi_device_queue_trans(state.dev, &trans, wait);
    }

    [[nodiscard]] static esp_err_t wait(
        spi_transaction_t** const trans,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        boot();
        return spi_device_get_trans_result(state.dev, trans, wait_ticks);
    }

    [[nodiscard]] static constexpr int cs() noexcept
    {
        return Cs;
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static constexpr std::uint8_t mode() noexcept
    {
        return Mode;
    }

    [[nodiscard]] static constexpr int depth() noexcept
    {
        return Queue;
    }

private:
    struct State {
        spi_device_handle_t dev{};
        bool held{};
    };

    constinit static inline State state{};
};

}  // namespace arc
