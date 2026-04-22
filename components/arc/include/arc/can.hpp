#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
// Preload pthread before ESP-IDF HAL defines __noreturn as a C++ attribute.
#include <pthread.h>
#include <span>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_err.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "hal/twai_types.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"
#include "arc/sdk.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"

namespace arc {

template <int Tx,
          int Rx,
          std::uint32_t Bitrate = 500'000,
          std::uint32_t Queue = 8,
          std::size_t RxDepth = 8,
          bool SelfTest = false,
          bool Loopback = false,
          bool Listen = false,
          int Quanta = -1,
          int BusOff = -1,
          std::uint32_t StampHz = 0,
          std::int8_t Retries = -1,
          int Intr = 0,
          twai_clock_source_t Source = TWAI_CLK_SRC_DEFAULT>
struct Can {
    static_assert(SOC_TWAI_SUPPORTED, "TWAI/CAN is not supported on this target");
    static_assert(Tx >= 0 && Tx < SOC_GPIO_PIN_COUNT, "invalid CAN TX pin");
    static_assert(Rx >= 0 && Rx < SOC_GPIO_PIN_COUNT, "invalid CAN RX pin");
    static_assert((Quanta >= 0 && Quanta < SOC_GPIO_PIN_COUNT) || Quanta == -1, "invalid CAN quanta output pin");
    static_assert((BusOff >= 0 && BusOff < SOC_GPIO_PIN_COUNT) || BusOff == -1, "invalid CAN bus-off output pin");
    static_assert(Bitrate != 0U, "CAN bitrate must be non-zero");
    static_assert(Queue != 0U, "CAN TX queue must be non-zero");

    struct Frame {
        twai_frame_header_t header{};
        std::array<std::uint8_t, TWAI_FRAME_MAX_LEN> data{};
        std::uint16_t bytes{};
    };

    static void boot()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        twai_onchip_node_config_t config{};
        config.io_cfg.tx = gpio(Tx);
        config.io_cfg.rx = gpio(Rx);
        config.io_cfg.quanta_clk_out = gpio_or_nc(Quanta);
        config.io_cfg.bus_off_indicator = gpio_or_nc(BusOff);
        config.clk_src = Source;
        config.bit_timing.bitrate = Bitrate;
        config.bit_timing.sp_permill = 0U;
        config.bit_timing.ssp_permill = 0U;
        config.timestamp_resolution_hz = StampHz;
        config.fail_retry_cnt = Retries;
        config.tx_queue_depth = Queue;
        config.intr_priority = Intr;
        config.flags.enable_self_test = SelfTest ? 1U : 0U;
        config.flags.enable_loopback = Loopback ? 1U : 0U;
        config.flags.enable_listen_only = Listen ? 1U : 0U;
        config.flags.no_receive_rtr = 0U;
        ESP_ERROR_CHECK(twai_new_node_onchip(&config, &state.node));

        twai_event_callbacks_t callbacks{};
        callbacks.on_tx_done = &on_tx;
        callbacks.on_rx_done = &on_rx;
        callbacks.on_state_change = &on_state;
        callbacks.on_error = &on_error;
        ESP_ERROR_CHECK(twai_node_register_event_callbacks(state.node, &callbacks, nullptr));
        Init::pass(state.init);
    }

    static void start()
    {
        boot();
        if (!state.enabled) {
            ESP_ERROR_CHECK(twai_node_enable(state.node));
            state.enabled = true;
        }
    }

    static void stop()
    {
        if (state.node == nullptr || !state.enabled) {
            return;
        }

        ESP_ERROR_CHECK(twai_node_disable(state.node));
        state.enabled = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        start();
    }

    template <std::size_t Extent>
    [[nodiscard]] static Frame frame(
        const std::uint32_t id,
        const std::span<const std::uint8_t, Extent> payload,
        const bool ext = false,
        const bool remote = false) noexcept
    {
        Frame out{};
        const auto size = std::min(payload.size(), out.data.size());
        out.header.id = id & (ext ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK);
        out.header.dlc = static_cast<std::uint16_t>(size);
        out.header.ide = ext ? 1U : 0U;
        out.header.rtr = remote ? 1U : 0U;
        out.header.fdf = 0U;
        out.header.brs = 0U;
        out.header.esi = 0U;
        out.bytes = static_cast<std::uint16_t>(size);
        if (size != 0U) {
            __builtin_memcpy(out.data.data(), payload.data(), size);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t send(Frame& frame, const int timeout_ms = 0) noexcept
    {
        start();

        auto native = native_frame(frame);
        const auto err = twai_node_transmit(state.node, &native, timeout_ms);
        if (err == ESP_OK) {
            __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
            __atomic_add_fetch(&state.bytes, frame.bytes, __ATOMIC_RELEASE);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t send_wait(Frame& frame, const int timeout_ms = 1000) noexcept
    {
        const auto target = sent() + 1U;
        const auto err = send(frame, timeout_ms);
        if (err != ESP_OK) {
            return err;
        }
        const auto waited = wait(timeout_ms);
        if (waited != ESP_OK) {
            return waited;
        }
        while (seq_before(done(), target)) {
            __asm__ __volatile__("nop");
        }
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t wait(const int timeout_ms = -1) noexcept
    {
        boot();
        return twai_node_transmit_wait_all_done(state.node, timeout_ms);
    }

    [[nodiscard]] static bool recv(Frame& frame) noexcept
    {
        return state.rx.try_pop(frame);
    }

    [[nodiscard]] static esp_err_t recover() noexcept
    {
        boot();
        return twai_node_recover(state.node);
    }

    [[nodiscard]] static esp_err_t info(
        twai_node_status_t* const status,
        twai_node_record_t* const record = nullptr) noexcept
    {
        boot();
        return twai_node_get_info(state.node, status, record);
    }

    [[nodiscard]] static esp_err_t filter(
        const std::uint8_t index,
        const twai_mask_filter_config_t& config) noexcept
    {
        boot();
        return twai_node_config_mask_filter(state.node, index, &config);
    }

    [[nodiscard]] static esp_err_t filter(
        const std::uint8_t index,
        const std::uint32_t id,
        const std::uint32_t mask,
        const bool ext = false,
        const bool no_classic = false,
        const bool no_fd = false) noexcept
    {
        twai_mask_filter_config_t config{};
        config.id = id & (ext ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK);
        config.mask = mask & (ext ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK);
        config.is_ext = ext ? 1U : 0U;
        config.no_classic = no_classic ? 1U : 0U;
        config.no_fd = no_fd ? 1U : 0U;
        config.dual_filter = 0U;
        return filter(index, config);
    }

    [[nodiscard]] static esp_err_t dual(
        const std::uint8_t index,
        const std::uint32_t id1,
        const std::uint32_t mask1,
        const std::uint32_t id2,
        const std::uint32_t mask2,
        const bool ext = false) noexcept
    {
        const auto config = twai_make_dual_filter(id1, mask1, id2, mask2, ext);
        return filter(index, config);
    }

    [[nodiscard]] static esp_err_t range(
        const std::uint8_t index,
        const twai_range_filter_config_t& config) noexcept
    {
        boot();
        return twai_node_config_range_filter(state.node, index, &config);
    }

    [[nodiscard]] static esp_err_t range(
        const std::uint8_t index,
        const std::uint32_t low,
        const std::uint32_t high,
        const bool ext = false,
        const bool no_classic = false,
        const bool no_fd = false) noexcept
    {
        twai_range_filter_config_t config{};
        config.range_low = low & (ext ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK);
        config.range_high = high & (ext ? TWAI_EXT_ID_MASK : TWAI_STD_ID_MASK);
        config.is_ext = ext ? 1U : 0U;
        config.no_classic = no_classic ? 1U : 0U;
        config.no_fd = no_fd ? 1U : 0U;
        return range(index, config);
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t done() noexcept
    {
        return __atomic_load_n(&state.done, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t rx() noexcept
    {
        return __atomic_load_n(&state.rx_count, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t drop() noexcept
    {
        return __atomic_load_n(&state.rx_drop, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t err() noexcept
    {
        return __atomic_load_n(&state.errors, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::size_t bytes() noexcept
    {
        return __atomic_load_n(&state.bytes, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static bool idle() noexcept
    {
        return seq_reached(done(), sent());
    }

    [[nodiscard]] static constexpr std::uint32_t bitrate() noexcept
    {
        return Bitrate;
    }

    [[nodiscard]] static twai_node_handle_t native()
    {
        boot();
        return state.node;
    }

private:
    struct State {
        twai_node_handle_t node{};
        Spsc<Frame, RxDepth> rx{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t done{};
        alignas(cache_line) std::uint32_t rx_count{};
        alignas(cache_line) std::uint32_t rx_drop{};
        alignas(cache_line) std::uint32_t errors{};
        std::size_t bytes{};
        bool enabled{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static twai_frame_t native_frame(Frame& frame) noexcept
    {
        twai_frame_t native{};
        native.header = frame.header;
        native.buffer = frame.data.data();
        native.buffer_len = std::min<std::size_t>(frame.bytes, frame.data.size());
        return native;
    }

    [[nodiscard]] static twai_frame_t native_rx(Frame& frame) noexcept
    {
        twai_frame_t native{};
        native.buffer = frame.data.data();
        native.buffer_len = frame.data.size();
        return native;
    }

    [[nodiscard]] static std::uint16_t payload_size(const twai_frame_header_t& header) noexcept
    {
        const auto size = header.fdf != 0U ? twaifd_dlc2len(header.dlc) : header.dlc;
        return static_cast<std::uint16_t>(std::min<std::uint16_t>(size, TWAI_FRAME_MAX_LEN));
    }

    [[nodiscard]] static constexpr gpio_num_t gpio(const int pin) noexcept
    {
        return static_cast<gpio_num_t>(pin);
    }

    [[nodiscard]] static constexpr gpio_num_t gpio_or_nc(const int pin) noexcept
    {
        return pin == -1 ? GPIO_NUM_NC : gpio(pin);
    }

    IRAM_ATTR static bool on_tx(
        twai_node_handle_t,
        const twai_tx_done_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.done, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_rx(
        twai_node_handle_t node,
        const twai_rx_done_event_data_t*,
        void*) noexcept
    {
        Frame frame{};
        auto native = native_rx(frame);
        if (twai_node_receive_from_isr(node, &native) != ESP_OK) {
            __atomic_add_fetch(&state.errors, 1U, __ATOMIC_RELEASE);
            return false;
        }

        frame.header = native.header;
        frame.bytes = payload_size(native.header);
        if (!state.rx.try_push(frame)) {
            __atomic_add_fetch(&state.rx_drop, 1U, __ATOMIC_RELEASE);
            return false;
        }

        __atomic_add_fetch(&state.rx_count, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_state(
        twai_node_handle_t,
        const twai_state_change_event_data_t*,
        void*) noexcept
    {
        return false;
    }

    IRAM_ATTR static bool on_error(
        twai_node_handle_t,
        const twai_error_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.errors, 1U, __ATOMIC_RELEASE);
        return false;
    }
};

}  // namespace arc
