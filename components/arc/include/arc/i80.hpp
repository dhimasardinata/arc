#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/cache.hpp"
#include "arc/caps.hpp"
#include "arc/init.hpp"
#include "arc/sdk.hpp"
#include "arc/seq.hpp"

namespace arc {

template <int... DataPins>
struct Lines {
    static constexpr std::size_t width = sizeof...(DataPins);
    static_assert(width == 8U || width == 16U, "I80 bus width must be 8 or 16");
    static_assert(((DataPins >= 0 && DataPins < SOC_GPIO_PIN_COUNT) && ...), "invalid I80 data pin");
};

template <typename DataLines,
          int Dc,
          int Wr,
          std::size_t MaxBytes = 4096,
          std::size_t BurstBytes = 64,
          lcd_clock_source_t Source = LCD_CLK_SRC_DEFAULT>
struct I80Bus;

template <int... DataPins,
          int Dc,
          int Wr,
          std::size_t MaxBytes,
          std::size_t BurstBytes,
          lcd_clock_source_t Source>
struct I80Bus<Lines<DataPins...>, Dc, Wr, MaxBytes, BurstBytes, Source> {
    static_assert(SOC_LCD_I80_SUPPORTED, "I80 LCD bus is not supported on this target");
    static_assert(Dc >= 0 && Dc < SOC_GPIO_PIN_COUNT, "invalid I80 DC pin");
    static_assert(Wr >= 0 && Wr < SOC_GPIO_PIN_COUNT, "invalid I80 WR pin");
    static_assert(MaxBytes > 0U, "I80 max transfer size must be non-zero");
    static_assert(BurstBytes > 0U, "I80 DMA burst size must be non-zero");
    static_assert(sizeof...(DataPins) == Lines<DataPins...>::width);

    static void boot()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        esp_lcd_i80_bus_config_t config{};
        config.dc_gpio_num = gpio(Dc);
        config.wr_gpio_num = gpio(Wr);
        config.clk_src = Source;
        config.bus_width = Lines<DataPins...>::width;
        config.max_transfer_bytes = MaxBytes;
        config.dma_burst_size = BurstBytes;

        std::size_t index = 0U;
        ((config.data_gpio_nums[index++] = gpio(DataPins)), ...);
        for (; index < ESP_LCD_I80_BUS_WIDTH_MAX; ++index) {
            config.data_gpio_nums[index] = GPIO_NUM_NC;
        }

        const auto err = esp_lcd_new_i80_bus(&config, &state.bus);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            state.bus = nullptr;
            Init::fail(state.init);
        }
        ESP_ERROR_CHECK(err);
    }

    [[nodiscard]] static constexpr std::size_t width() noexcept
    {
        return Lines<DataPins...>::width;
    }

    [[nodiscard]] static constexpr std::size_t max_bytes() noexcept
    {
        return MaxBytes;
    }

    [[nodiscard]] static constexpr int dc() noexcept
    {
        return Dc;
    }

    [[nodiscard]] static constexpr int wr() noexcept
    {
        return Wr;
    }

    [[nodiscard]] static esp_lcd_i80_bus_handle_t native()
    {
        boot();
        return state.bus;
    }

private:
    struct State {
        esp_lcd_i80_bus_handle_t bus{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static constexpr gpio_num_t gpio(const int pin) noexcept
    {
        return static_cast<gpio_num_t>(pin);
    }
};

template <typename Bus,
          int Cs,
          std::uint32_t Hz = 20'000'000,
          std::size_t Queue = 8,
          int CmdBits = 8,
          int ParamBits = 8,
          bool SwapBytes = false,
          bool ReverseBits = false,
          bool PclkNeg = false>
struct I80 {
    static_assert((Cs >= 0 && Cs < SOC_GPIO_PIN_COUNT) || Cs == -1, "invalid I80 CS pin");
    static_assert(Hz != 0U, "I80 pixel clock must be non-zero");
    static_assert(Queue > 0U, "I80 queue depth must be non-zero");
    static_assert(CmdBits > 0, "I80 command width must be non-zero");
    static_assert(ParamBits > 0, "I80 parameter width must be non-zero");

    struct Ticket {
        std::uint32_t target{};
    };

    static void boot()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        Bus::boot();

        esp_lcd_panel_io_i80_config_t config{};
        config.cs_gpio_num = static_cast<gpio_num_t>(Cs);
        config.pclk_hz = Hz;
        config.trans_queue_depth = Queue;
        config.on_color_trans_done = &on_done;
        config.user_ctx = nullptr;
        config.lcd_cmd_bits = CmdBits;
        config.lcd_param_bits = ParamBits;
        config.dc_levels.dc_idle_level = 0U;
        config.dc_levels.dc_cmd_level = 0U;
        config.dc_levels.dc_dummy_level = 0U;
        config.dc_levels.dc_data_level = 1U;
        config.flags.cs_active_high = 0U;
        config.flags.reverse_color_bits = ReverseBits ? 1U : 0U;
        config.flags.swap_color_bytes = SwapBytes ? 1U : 0U;
        config.flags.pclk_active_neg = PclkNeg ? 1U : 0U;
        config.flags.pclk_idle_low = 0U;
        const auto err = esp_lcd_new_panel_io_i80(Bus::native(), &config, &state.io);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            state.io = nullptr;
            Init::fail(state.init);
        }
        ESP_ERROR_CHECK(err);
    }

    [[nodiscard]] static esp_err_t param(
        const int cmd,
        const void* const data = nullptr,
        const std::size_t bytes = 0U) noexcept
    {
        boot();
        return esp_lcd_panel_io_tx_param(state.io, cmd, data, bytes);
    }

    [[nodiscard]] static esp_err_t color(
        const int cmd,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return color_impl(cmd, data, bytes, nullptr);
    }

    [[nodiscard]] static esp_err_t color(
        Ticket& ticket,
        const int cmd,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        return color_impl(cmd, data, bytes, &ticket);
    }

    template <typename T>
    [[nodiscard]] static esp_err_t color(const int cmd, const CapsBuf<T>& data) noexcept
    {
        return color(cmd, data.data(), data.bytes());
    }

    template <typename T>
    [[nodiscard]] static esp_err_t color(Ticket& ticket, const int cmd, const CapsBuf<T>& data) noexcept
    {
        return color(ticket, cmd, data.data(), data.bytes());
    }

    [[nodiscard]] static esp_err_t color_coherent(
        Ticket& ticket,
        const int cmd,
        void* const data,
        const std::size_t bytes) noexcept
    {
        const auto err = Cache::to_device(data, bytes);
        if (err != ESP_OK) {
            return err;
        }

        return color(ticket, cmd, data, bytes);
    }

    template <typename T>
    [[nodiscard]] static esp_err_t color_coherent(
        Ticket& ticket,
        const int cmd,
        CapsBuf<T>& data) noexcept
    {
        return color_coherent(ticket, cmd, data.data(), data.bytes());
    }

    [[nodiscard]] static esp_err_t color_coherent(
        const int cmd,
        void* const data,
        const std::size_t bytes) noexcept
    {
        Ticket ticket{};
        const auto err = color_coherent(ticket, cmd, data, bytes);
        if (err != ESP_OK) {
            return err;
        }
        finish(ticket);
        return ESP_OK;
    }

    template <typename T>
    [[nodiscard]] static esp_err_t color_coherent(const int cmd, CapsBuf<T>& data) noexcept
    {
        return color_coherent(cmd, data.data(), data.bytes());
    }

    template <typename T>
    [[nodiscard]] static CapsBuf<T> buffer(
        const std::size_t count,
        const std::uint32_t caps = static_cast<std::uint32_t>(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)) noexcept
    {
        boot();
        if (count == 0U) {
            return {};
        }

        void* const data = esp_lcd_i80_alloc_draw_buffer(state.io, count * sizeof(T), caps);
        return {static_cast<T*>(data), data == nullptr ? 0U : count};
    }

    [[nodiscard]] static std::uint32_t sent() noexcept
    {
        return __atomic_load_n(&state.sent, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t done() noexcept
    {
        return __atomic_load_n(&state.done, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::size_t bytes() noexcept
    {
        return __atomic_load_n(&state.bytes, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static bool idle() noexcept
    {
        return seq_reached(done(), sent());
    }

    [[nodiscard]] static bool ready(const Ticket& ticket) noexcept
    {
        return seq_reached(done(), ticket.target);
    }

    static void wait() noexcept
    {
        wait(sent());
    }

    static void wait(const Ticket& ticket) noexcept
    {
        wait(ticket.target);
    }

    static void finish(const Ticket& ticket) noexcept
    {
        wait(ticket);
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static constexpr int cs() noexcept
    {
        return Cs;
    }

    [[nodiscard]] static esp_lcd_panel_io_handle_t native()
    {
        boot();
        return state.io;
    }

private:
    struct State {
        esp_lcd_panel_io_handle_t io{};
        std::uint32_t init{};
        std::uint32_t gate{};
        alignas(cache_line) std::uint32_t sent{};
        alignas(cache_line) std::uint32_t done{};
        std::size_t bytes{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_done(
        esp_lcd_panel_io_handle_t,
        esp_lcd_panel_io_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.done, 1U, __ATOMIC_RELEASE);
        return false;
    }

    [[nodiscard]] static esp_err_t color_impl(
        const int cmd,
        const void* const data,
        const std::size_t bytes,
        Ticket* const ticket) noexcept
    {
        boot();
        Gate guard(state.gate);
        const auto err = esp_lcd_panel_io_tx_color(state.io, cmd, data, bytes);
        if (err == ESP_OK) {
            const auto target = __atomic_add_fetch(&state.sent, 1U, __ATOMIC_RELEASE);
            __atomic_add_fetch(&state.bytes, bytes, __ATOMIC_RELEASE);
            if (ticket != nullptr) {
                *ticket = Ticket{.target = target};
            }
        }
        return err;
    }

    static void wait(const std::uint32_t target) noexcept
    {
        while (seq_before(done(), target)) {
            __asm__ __volatile__("nop");
        }
    }
};

}  // namespace arc
