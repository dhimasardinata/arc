#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "driver/spi_slave.h"
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
concept SpiSlaveBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

template <spi_host_device_t Host,
          int Sclk,
          int Mosi,
          int Miso,
          int Cs,
          int Queue = 4,
          std::uint8_t Mode = 0,
          int MaxBytes = 4096,
          spi_dma_chan_t Dma = SPI_DMA_CH_AUTO,
          std::uint32_t BusFlags = SPICOMMON_BUSFLAG_SLAVE,
          std::uint32_t SlaveFlags = 0U,
          esp_intr_cpu_affinity_t IsrCpu = ESP_INTR_CPU_AFFINITY_0,
          int IntrFlags = 0>
struct SpiSlave {
    static_assert(Host == SPI2_HOST || Host == SPI3_HOST, "SPI slave host must be SPI2_HOST or SPI3_HOST");
    static_assert(Sclk >= 0 && Sclk < SOC_GPIO_PIN_COUNT, "invalid SPI slave SCLK pin");
    static_assert((Mosi >= 0 && Mosi < SOC_GPIO_PIN_COUNT) || Mosi == -1, "invalid SPI slave MOSI pin");
    static_assert((Miso >= 0 && Miso < SOC_GPIO_PIN_COUNT) || Miso == -1, "invalid SPI slave MISO pin");
    static_assert(Cs >= 0 && Cs < SOC_GPIO_PIN_COUNT, "invalid SPI slave CS pin");
    static_assert(Mosi != -1 || Miso != -1, "SPI slave needs MOSI, MISO, or both");
    static_assert(Queue > 0, "SPI slave queue depth must be non-zero");
    static_assert(Mode < 4U, "SPI slave mode must be in [0, 3]");
    static_assert(MaxBytes > 0, "SPI slave max transfer must be non-zero");
    static_assert((SlaveFlags & SPI_SLAVE_NO_RETURN_RESULT) == 0U,
                  "SpiSlave ticket ownership requires returned transaction results");

    inline constexpr static std::uint32_t ticket_pending = 0U;
    inline constexpr static std::uint32_t ticket_done = 1U;
    inline constexpr static std::uint32_t ticket_invalid = 2U;

    using Transaction = spi_slave_transaction_t;

    struct TicketBase {
        std::uint32_t done{ticket_invalid};
        Transaction trans{};
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

        auto err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        spi_bus_config_t bus{};
        bus.sclk_io_num = Sclk;
        bus.mosi_io_num = Mosi;
        bus.miso_io_num = Miso;
        bus.quadwp_io_num = -1;
        bus.quadhd_io_num = -1;
        bus.data4_io_num = -1;
        bus.data5_io_num = -1;
        bus.data6_io_num = -1;
        bus.data7_io_num = -1;
        bus.data_io_default_level = false;
        bus.max_transfer_sz = MaxBytes;
        bus.flags = BusFlags;
        bus.isr_cpu_id = IsrCpu;
        bus.intr_flags = IntrFlags;

        spi_slave_interface_config_t dev{};
        dev.spics_io_num = Cs;
        dev.flags = SlaveFlags;
        dev.queue_size = Queue;
        dev.mode = Mode;
        dev.post_setup_cb = nullptr;
        dev.post_trans_cb = nullptr;

        err = spi_slave_initialize(Host, &bus, &dev, Dma);
        if (err == ESP_OK) {
            state.enabled = true;
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

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        if (!state.enabled) {
            const auto err = spi_slave_enable(Host);
            if (err != ESP_OK) {
                return err;
            }
            state.enabled = true;
        }
        return ESP_OK;
    }

    static void start()
    {
        ESP_ERROR_CHECK(enable());
    }

    [[nodiscard]] static esp_err_t disable() noexcept
    {
        if (!Resource::held() || !state.enabled) {
            return ESP_OK;
        }

        const auto err = spi_slave_disable(Host);
        if (err != ESP_OK) {
            return err;
        }
        state.enabled = false;
        return ESP_OK;
    }

    static void stop()
    {
        ESP_ERROR_CHECK(disable());
    }

    [[nodiscard]] static Transaction job(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const void* const user = nullptr,
        const std::uint32_t flags = 0U) noexcept
    {
        Transaction trans{};
        trans.flags = flags;
        trans.length = bit_length(bytes);
        trans.tx_buffer = tx;
        trans.rx_buffer = rx;
        trans.user = const_cast<void*>(user);
        return trans;
    }

    [[nodiscard]] static esp_err_t send(
        const void* const tx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        auto trans = job(tx, nullptr, bytes);
        return xfer(trans, wait);
    }

    [[nodiscard]] static esp_err_t recv(
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        auto trans = job(nullptr, rx, bytes);
        return xfer(trans, wait);
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        auto trans = job(tx, rx, bytes);
        return xfer(trans, wait);
    }

    template <typename T, std::size_t Extent>
        requires SpiSlaveBytes<T>
    [[nodiscard]] static esp_err_t send(
        const std::span<T, Extent> tx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return send(tx.data(), tx.size_bytes(), wait);
    }

    template <typename T, std::size_t Extent>
        requires(SpiSlaveBytes<T> && !std::is_const_v<T>)
    [[nodiscard]] static esp_err_t recv(
        const std::span<T, Extent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return recv(rx.data(), rx.size_bytes(), wait);
    }

    template <typename Tx, std::size_t TxExtent, typename Rx, std::size_t RxExtent>
        requires(SpiSlaveBytes<Tx> && SpiSlaveBytes<Rx> && !std::is_const_v<Rx>)
    [[nodiscard]] static esp_err_t xfer(
        const std::span<Tx, TxExtent> tx,
        const std::span<Rx, RxExtent> rx,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (tx.size_bytes() != rx.size_bytes()) {
            return ESP_ERR_INVALID_ARG;
        }

        return xfer(tx.data(), rx.data(), tx.size_bytes(), wait);
    }

    [[nodiscard]] static esp_err_t xfer(
        Transaction& trans,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (invalid(trans)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return spi_slave_transmit(Host, &trans, wait);
    }

    [[nodiscard]] static esp_err_t xfer_coherent(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return xfer_cache<false>(tx, rx, bytes, wait);
    }

    [[nodiscard]] static esp_err_t xfer_coherent_strict(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        return xfer_cache<true>(tx, rx, bytes, wait);
    }

    [[nodiscard]] static esp_err_t queue(
        Transaction& trans,
        const TickType_t wait = portMAX_DELAY) noexcept
    {
        if (invalid(trans)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        return spi_slave_queue_trans(Host, &trans, wait);
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

    [[nodiscard]] static esp_err_t wait(
        Transaction*& trans,
        const TickType_t wait_ticks = portMAX_DELAY) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        Transaction* done{};
        const auto err = spi_slave_get_trans_result(Host, &done, wait_ticks);
        if (err == ESP_OK) {
            trans = done;
            static_cast<void>(mark_done(done));
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

    [[nodiscard]] static constexpr int cs() noexcept
    {
        return Cs;
    }

    [[nodiscard]] static constexpr int depth() noexcept
    {
        return Queue;
    }

    [[nodiscard]] static constexpr int max_bytes() noexcept
    {
        return MaxBytes;
    }

    [[nodiscard]] static constexpr bool bytes_fit(const std::size_t bytes) noexcept
    {
        return bytes != 0U &&
            bytes <= static_cast<std::size_t>(MaxBytes) &&
            bytes <= std::numeric_limits<std::size_t>::max() / 8U;
    }

    [[nodiscard]] static bool enabled() noexcept
    {
        return state.enabled;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = spi_slave_free(Host);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.enabled = false;
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
                              Cs,
                              Queue,
                              Mode,
                              MaxBytes,
                              Dma,
                              BusFlags,
                              SlaveFlags,
                              IsrCpu,
                              IntrFlags>;

    struct State {
        bool enabled;
        std::uint32_t init;
    };

    constinit static inline State state{false, 0U};
    constinit static inline std::uint32_t ticket_cookie{};

    [[nodiscard]] static constexpr std::size_t bit_length(const std::size_t bytes) noexcept
    {
        return bytes_fit(bytes) ? bytes * 8U : 0U;
    }

    [[nodiscard]] static bool invalid(const Transaction& trans) noexcept
    {
        if (trans.length == 0U || (trans.tx_buffer == nullptr && trans.rx_buffer == nullptr)) {
            return true;
        }

        const auto bytes = (trans.length + 7U) / 8U;
        return bytes > static_cast<std::size_t>(MaxBytes);
    }

    [[nodiscard]] static void* ticket_marker() noexcept
    {
        return &ticket_cookie;
    }

    [[nodiscard]] static bool mark_done(Transaction* const trans) noexcept
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
    [[nodiscard]] static esp_err_t sync_before(
        const void* const tx,
        void* const rx,
        const std::size_t bytes) noexcept
    {
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

        return ESP_OK;
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t sync_after(void* const rx, const std::size_t bytes) noexcept
    {
        if (rx == nullptr || bytes == 0U) {
            return ESP_OK;
        }

        if constexpr (Strict) {
            return Cache::from_aligned(rx, bytes);
        } else {
            return Cache::from_device(rx, bytes);
        }
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t xfer_cache(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait) noexcept
    {
        if (bytes == 0U || bytes > static_cast<std::size_t>(MaxBytes) || (tx == nullptr && rx == nullptr)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto before = sync_before<Strict>(tx, rx, bytes);
        if (before != ESP_OK) {
            return before;
        }

        auto trans = job(tx, rx, bytes);
        const auto err = xfer(trans, wait);
        if (err != ESP_OK) {
            return err;
        }

        return sync_after<Strict>(rx, bytes);
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t queue_impl(
        Ticket<Strict>& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait) noexcept
    {
        if (bytes == 0U || bytes > static_cast<std::size_t>(MaxBytes) || (tx == nullptr && rx == nullptr)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        __atomic_store_n(&ticket.done, ticket_pending, __ATOMIC_RELAXED);
        ticket.trans = job(tx, rx, bytes, ticket_marker());
        ticket.rx = rx;
        ticket.rx_bytes = rx == nullptr ? 0U : bytes;
        const auto err = spi_slave_queue_trans(Host, &ticket.trans, wait);
        if (err != ESP_OK) {
            __atomic_store_n(&ticket.done, ticket_invalid, __ATOMIC_RELEASE);
        }
        return err;
    }

    template <bool Strict>
    [[nodiscard]] static esp_err_t queue_cache(
        Ticket<Strict>& ticket,
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const TickType_t wait) noexcept
    {
        if (bytes == 0U || bytes > static_cast<std::size_t>(MaxBytes) || (tx == nullptr && rx == nullptr)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto before = sync_before<Strict>(tx, rx, bytes);
        if (before != ESP_OK) {
            return before;
        }

        return queue_impl(ticket, tx, rx, bytes, wait);
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

            Transaction* done{};
            const auto err = wait(done, remaining);
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
    [[nodiscard]] static esp_err_t finish_cache(
        Ticket<Strict>& ticket,
        const TickType_t wait_ticks) noexcept
    {
        const auto err = finish_wait(ticket, wait_ticks);
        if (err != ESP_OK) {
            return err;
        }

        return sync_after<Strict>(ticket.rx, ticket.rx_bytes);
    }
};

}  // namespace arc
