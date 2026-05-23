#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_intr_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "arc/cache.hpp"
#include "arc/claim.hpp"
#include "arc/init.hpp"

namespace arc {

template <typename T>
concept SpiBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <spi_host_device_t Host,
          int Sclk,
          int Mosi,
          int Miso = -1,
          int MaxBytes = 4096,
          spi_dma_chan_t Dma = SPI_DMA_CH_AUTO,
          std::uint32_t Flags = 0U,
          esp_intr_cpu_affinity_t IsrCpu = ESP_INTR_CPU_AFFINITY_0,
          int IntrFlags = 0,
          int QuadWp = -1,
          int QuadHd = -1,
          int Data4 = -1,
          int Data5 = -1,
          int Data6 = -1,
          int Data7 = -1,
          bool DataDefault = false>
struct SpiBus {
    static_assert(Sclk >= 0 && Sclk < SOC_GPIO_PIN_COUNT, "invalid SPI SCLK pin");
    static_assert((Mosi >= 0 && Mosi < SOC_GPIO_PIN_COUNT) || Mosi == -1, "invalid SPI MOSI pin");
    static_assert((Miso >= 0 && Miso < SOC_GPIO_PIN_COUNT) || Miso == -1, "invalid SPI MISO pin");
    static_assert((QuadWp >= 0 && QuadWp < SOC_GPIO_PIN_COUNT) || QuadWp == -1, "invalid SPI WP pin");
    static_assert((QuadHd >= 0 && QuadHd < SOC_GPIO_PIN_COUNT) || QuadHd == -1, "invalid SPI HD pin");
    static_assert((Data4 >= 0 && Data4 < SOC_GPIO_PIN_COUNT) || Data4 == -1, "invalid SPI data4 pin");
    static_assert((Data5 >= 0 && Data5 < SOC_GPIO_PIN_COUNT) || Data5 == -1, "invalid SPI data5 pin");
    static_assert((Data6 >= 0 && Data6 < SOC_GPIO_PIN_COUNT) || Data6 == -1, "invalid SPI data6 pin");
    static_assert((Data7 >= 0 && Data7 < SOC_GPIO_PIN_COUNT) || Data7 == -1, "invalid SPI data7 pin");
    static_assert(Mosi != -1 || Miso != -1, "SPI bus needs MOSI, MISO, or both");
    static_assert((QuadWp == -1 && QuadHd == -1) || (QuadWp != -1 && QuadHd != -1), "quad SPI needs both WP and HD pins");
    static_assert(
        (Data4 == -1 && Data5 == -1 && Data6 == -1 && Data7 == -1) ||
            (Data4 != -1 && Data5 != -1 && Data6 != -1 && Data7 != -1),
        "octal SPI needs all data4..data7 pins");
    static_assert(
        Data4 == -1 || (QuadWp != -1 && QuadHd != -1),
        "octal SPI also needs the quad WP and HD pins");
    static_assert(MaxBytes > 0, "SPI max transfer must be non-zero");

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

        spi_bus_config_t cfg{};
        cfg.sclk_io_num = Sclk;
        cfg.mosi_io_num = Mosi;
        cfg.miso_io_num = Miso;
        cfg.quadwp_io_num = QuadWp;
        cfg.quadhd_io_num = QuadHd;
        cfg.data4_io_num = Data4;
        cfg.data5_io_num = Data5;
        cfg.data6_io_num = Data6;
        cfg.data7_io_num = Data7;
        cfg.max_transfer_sz = MaxBytes;
        cfg.flags = Flags;
        cfg.isr_cpu_id = IsrCpu;
        cfg.intr_flags = IntrFlags;
        cfg.data_io_default_level = DataDefault;
        err = spi_bus_initialize(Host, &cfg, Dma);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            Resource::drop();
            Init::fail(state.init);
        }
        return err;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
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

    [[nodiscard]] static constexpr int quadwp() noexcept
    {
        return QuadWp;
    }

    [[nodiscard]] static constexpr int quadhd() noexcept
    {
        return QuadHd;
    }

    [[nodiscard]] static constexpr bool quad() noexcept
    {
        return QuadWp != -1 && QuadHd != -1;
    }

    [[nodiscard]] static constexpr bool octal() noexcept
    {
        return Data4 != -1;
    }

    [[nodiscard]] static constexpr int max_bytes() noexcept
    {
        return MaxBytes;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = spi_bus_free(Host);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

private:
    using Resource = ClaimFor<ClaimKind::spi_bus,
                              static_cast<int>(Host),
                              Host,
                              Sclk,
                              Mosi,
                              Miso,
                              MaxBytes,
                              Dma,
                              Flags,
                              IsrCpu,
                              IntrFlags,
                              QuadWp,
                              QuadHd,
                              Data4,
                              Data5,
                              Data6,
                              Data7,
                              DataDefault>;

    struct State {
        std::uint32_t init;
    };

    constinit static inline State state{0U};
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

    inline constexpr static std::uint32_t ticket_pending = 0U;
    inline constexpr static std::uint32_t ticket_done = 1U;
    inline constexpr static std::uint32_t ticket_invalid = 2U;
    inline constexpr static std::uint32_t dual = SPI_TRANS_MODE_DIO;
    inline constexpr static std::uint32_t quad = SPI_TRANS_MODE_QIO;
    inline constexpr static std::uint32_t octal = SPI_TRANS_MODE_OCT;

    [[nodiscard]] static constexpr bool bytes_fit(const std::size_t bytes) noexcept
    {
        return bytes != 0U &&
            bytes <= static_cast<std::size_t>(Bus::max_bytes()) &&
            bytes <= std::numeric_limits<std::size_t>::max() / 8U;
    }

    struct TicketBase {
        std::uint32_t done{ticket_invalid};
        spi_transaction_t trans{};
        void* rx{};
        std::size_t rx_bytes{};
    };

    template <bool Strict = false>
    struct Ticket : TicketBase {};

    using Move = Ticket<false>;
    using StrictMove = Ticket<true>;

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Bus::init();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        if constexpr (Cs >= 0) {
            err = Resource::take();
            if (err != ESP_OK) {
                Init::fail(state.init);
                return err;
            }
        }

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
        err = spi_bus_add_device(Bus::host(), &cfg, &state.dev);
        if (err != ESP_OK) {
            if constexpr (Cs >= 0) {
                Resource::drop();
            }
            state.dev = nullptr;
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

    static void acquire(const TickType_t wait = portMAX_DELAY)
    {
        ESP_ERROR_CHECK(hold(wait));
    }

    [[nodiscard]] static esp_err_t hold(const TickType_t wait = portMAX_DELAY) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        const auto task = xTaskGetCurrentTaskHandle();
        if (__atomic_load_n(&state.owner, __ATOMIC_ACQUIRE) == task) {
            __atomic_add_fetch(&state.depth, 1U, __ATOMIC_RELAXED);
            return ESP_OK;
        }

        const auto err = spi_device_acquire_bus(state.dev, wait);
        if (err == ESP_OK) {
            __atomic_store_n(&state.depth, 1U, __ATOMIC_RELEASE);
            __atomic_store_n(&state.owner, task, __ATOMIC_RELEASE);
        }
        return err;
    }

    static void release() noexcept
    {
        if (state.dev == nullptr) {
            return;
        }

        const auto task = xTaskGetCurrentTaskHandle();
        if (__atomic_load_n(&state.owner, __ATOMIC_ACQUIRE) != task) {
            return;
        }

        if (__atomic_sub_fetch(&state.depth, 1U, __ATOMIC_RELEASE) != 0U) {
            return;
        }

        __atomic_store_n(&state.owner, static_cast<TaskHandle_t>(nullptr), __ATOMIC_RELEASE);
        spi_device_release_bus(state.dev);
    }

    [[nodiscard]] static spi_transaction_t job(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) noexcept
    {
        spi_transaction_t trans{};
        trans.flags = flags;
        const auto bits = bit_length(bytes);
        trans.length = bits;
        trans.rxlength = rx == nullptr ? 0U : bits;
        trans.override_freq_hz = static_cast<decltype(trans.override_freq_hz)>(hz);
        trans.user = nullptr;
        trans.tx_buffer = tx;
        trans.rx_buffer = rx;
        return trans;
    }

    [[nodiscard]] static esp_err_t send(
        const void* const tx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) noexcept
    {
        if (tx == nullptr || !bytes_fit(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        auto trans = job(tx, nullptr, bytes, hz, flags);
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t recv(
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) noexcept
    {
        if (rx == nullptr || !bytes_fit(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        auto trans = job(nullptr, rx, bytes, hz, flags);
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) noexcept
    {
        if ((tx == nullptr && rx == nullptr) || !bytes_fit(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        auto trans = job(tx, rx, bytes, hz, flags);
        return spi_device_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t poll(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t hz = 0U,
        const std::uint32_t flags = 0U) noexcept
    {
        if ((tx == nullptr && rx == nullptr) || !bytes_fit(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        auto trans = job(tx, rx, bytes, hz, flags);
        return spi_device_polling_transmit(state.dev, &trans);
    }

    [[nodiscard]] static esp_err_t queue(
        spi_transaction_t& trans,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (invalid(trans)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return spi_device_queue_trans(state.dev, &trans, wait);
    }

    [[nodiscard]] static esp_err_t queue(
        Move& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_impl(ticket, tx, rx, bytes, wait);
    }

    [[nodiscard]] static esp_err_t queue_strict(
        StrictMove& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_impl(ticket, tx, rx, bytes, wait);
    }

    template <typename T, std::size_t Extent>
        requires SpiBytes<T>
    [[nodiscard]] static esp_err_t queue(
        Move& ticket,
        const std::span<T, Extent> tx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue(ticket, tx.data(), nullptr, tx.size_bytes(), wait);
    }

    template <typename T, std::size_t Extent>
        requires SpiBytes<T>
    [[nodiscard]] static esp_err_t queue_strict(
        StrictMove& ticket,
        const std::span<T, Extent> tx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_strict(ticket, tx.data(), nullptr, tx.size_bytes(), wait);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(SpiBytes<Tx> && SpiBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t queue(
        Move& ticket,
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }

        return queue(ticket, tx.data(), rx.data(), tx.size_bytes(), wait);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(SpiBytes<Tx> && SpiBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t queue_strict(
        StrictMove& ticket,
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }

        return queue_strict(ticket, tx.data(), rx.data(), tx.size_bytes(), wait);
    }

    [[nodiscard]] static esp_err_t wait(
        spi_transaction_t** const trans,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        const auto err = spi_device_get_trans_result(state.dev, trans, wait_ticks);
        if (err == ESP_OK && trans != nullptr) {
            static_cast<void>(mark_done(*trans));
        }
        return err;
    }

    [[nodiscard]] static esp_err_t finish(
        Move& ticket,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        return finish_wait(ticket, wait_ticks);
    }

    [[nodiscard]] static esp_err_t finish(
        StrictMove& ticket,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        return finish_wait(ticket, wait_ticks);
    }

    [[nodiscard]] static esp_err_t queue_coherent(
        Move& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_cache(ticket, tx, rx, bytes, wait);
    }

    [[nodiscard]] static esp_err_t queue_aligned(
        StrictMove& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_cache(ticket, tx, rx, bytes, wait);
    }

    template <typename T, std::size_t Extent>
        requires SpiBytes<T>
    [[nodiscard]] static esp_err_t queue_coherent(
        Move& ticket,
        const std::span<T, Extent> tx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_coherent(ticket, tx.data(), nullptr, tx.size_bytes(), wait);
    }

    template <typename T, std::size_t Extent>
        requires SpiBytes<T>
    [[nodiscard]] static esp_err_t queue_aligned(
        StrictMove& ticket,
        const std::span<T, Extent> tx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return queue_aligned(ticket, tx.data(), nullptr, tx.size_bytes(), wait);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(SpiBytes<Tx> && SpiBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t queue_coherent(
        Move& ticket,
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }

        return queue_coherent(ticket, tx.data(), rx.data(), tx.size_bytes(), wait);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(SpiBytes<Tx> && SpiBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t queue_aligned(
        StrictMove& ticket,
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }

        return queue_aligned(ticket, tx.data(), rx.data(), tx.size_bytes(), wait);
    }

    [[nodiscard]] static esp_err_t finish_coherent(
        Move& ticket,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        return finish_cache(ticket, wait_ticks);
    }

    [[nodiscard]] static esp_err_t finish_coherent(
        StrictMove& ticket,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        return finish_cache(ticket, wait_ticks);
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

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        release();
        const auto err = spi_bus_remove_device(state.dev);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.dev = nullptr;
        state.owner = nullptr;
        state.depth = 0U;
        if constexpr (Cs >= 0) {
            Resource::drop();
        }
        Init::fail(state.init);
        return ESP_OK;
    }

private:
    using Resource = ClaimFor<ClaimKind::spi_dev,
                              (static_cast<int>(Bus::host()) * (SOC_GPIO_PIN_COUNT + 1)) + Cs + 1,
                              Bus::host(),
                              Cs,
                              Hz,
                              Mode,
                              Queue,
                              Flags,
                              Clock,
                              DutyCyclePos,
                              InputDelayNs>;

    struct State {
        spi_device_handle_t dev;
        TaskHandle_t owner;
        std::uint32_t depth;
        std::uint32_t init;
    };

    constinit static inline State state{nullptr, nullptr, 0U, 0U};
    constinit static inline std::uint32_t ticket_cookie{};

    [[nodiscard]] static void* ticket_marker() noexcept
    {
        return &ticket_cookie;
    }

    [[nodiscard]] static constexpr std::size_t bit_length(const std::size_t bytes) noexcept
    {
        return bytes_fit(bytes) ? bytes * 8U : 0U;
    }

    [[nodiscard]] static bool invalid(const spi_transaction_t& trans) noexcept
    {
        if (trans.length == 0U || (trans.tx_buffer == nullptr && trans.rx_buffer == nullptr)) {
            return true;
        }
        const auto bytes = (trans.length + 7U) / 8U;
        return bytes > static_cast<std::size_t>(Bus::max_bytes());
    }

    [[nodiscard]] static bool mark_done(spi_transaction_t* const trans) noexcept
    {
        if (trans == nullptr || trans->user != ticket_marker()) {
            return false;
        }

        auto* const raw = reinterpret_cast<std::uint8_t*>(trans);
        auto* const ticket = reinterpret_cast<TicketBase*>(raw - offsetof(TicketBase, trans));
        __atomic_store_n(&ticket->done, ticket_done, __ATOMIC_RELEASE);
        return true;
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t queue_impl(
        Ticket<Strict>& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait) noexcept
    {
        if ((tx == nullptr && rx == nullptr) || !bytes_fit(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        __atomic_store_n(&ticket.done, ticket_pending, __ATOMIC_RELAXED);
        ticket.trans = job(tx, rx, bytes);
        ticket.trans.user = ticket_marker();
        ticket.rx = rx;
        ticket.rx_bytes = rx == nullptr ? 0U : bytes;
        const auto err = spi_device_queue_trans(state.dev, &ticket.trans, wait);
        if (err != ESP_OK) {
            __atomic_store_n(&ticket.done, ticket_invalid, __ATOMIC_RELEASE);
        }
        return err;
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t finish_wait(
        Ticket<Strict>& ticket,
        const TickType_t wait_ticks) noexcept
    {
        const auto start = xTaskGetTickCount();
        auto remaining = wait_ticks;

        for (;;) {
            const auto status = __atomic_load_n(&ticket.done, __ATOMIC_ACQUIRE);
            if (status == ticket_done) {
                return ESP_OK;
            }
            if (status != ticket_pending) {
                return ESP_ERR_INVALID_STATE;
            }

            spi_transaction_t* done{};
            const auto err = wait(&done, remaining);
            if (err != ESP_OK) {
                return err;
            }
            if (!mark_done(done)) {
                return ESP_ERR_INVALID_STATE;
            }

            if (wait_ticks != portMAX_DELAY &&
                __atomic_load_n(&ticket.done, __ATOMIC_ACQUIRE) == ticket_pending) {
                const auto elapsed = static_cast<TickType_t>(xTaskGetTickCount() - start);
                if (elapsed >= wait_ticks) {
                    return ESP_ERR_TIMEOUT;
                }
                remaining = static_cast<TickType_t>(wait_ticks - elapsed);
            }
        }
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t queue_cache(
        Ticket<Strict>& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait) noexcept
    {
        if (bytes == 0U || (tx == nullptr && rx == nullptr)) {
            return ESP_ERR_INVALID_ARG;
        }

        if (tx != nullptr) {
            const auto source = [&]() noexcept -> esp_err_t {
                if constexpr (Strict) {
                    return Cache::to_aligned(const_cast<void*>(tx), bytes);
                } else {
                    return Cache::to_device(const_cast<void*>(tx), bytes);
                }
            }();
            if (source != ESP_OK) {
                return source;
            }
        }

        if (rx != nullptr) {
            const auto target = [&]() noexcept -> esp_err_t {
                if constexpr (Strict) {
                    return Cache::discard_aligned(rx, bytes);
                } else {
                    return Cache::discard(rx, bytes);
                }
            }();
            if (target != ESP_OK) {
                return target;
            }
        }

        return queue_impl(ticket, tx, rx, bytes, wait);
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t finish_cache(
        Ticket<Strict>& ticket,
        const TickType_t wait_ticks) noexcept
    {
        const auto err = finish_wait(ticket, wait_ticks);
        if (err != ESP_OK || ticket.rx == nullptr || ticket.rx_bytes == 0U) {
            return err;
        }

        if constexpr (Strict) {
            return Cache::from_aligned(ticket.rx, ticket.rx_bytes);
        } else {
            return Cache::from_device(ticket.rx, ticket.rx_bytes);
        }
    }
};

}  // namespace arc
