#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "esp_cam_ctlr.h"
#include "esp_cam_ctlr_dvp.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "hal/color_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/caps.hpp"
#include "arc/init.hpp"

namespace arc {

template <int... DataPins>
struct DvpLines {
    static constexpr std::size_t width = sizeof...(DataPins);
    static_assert(width == 8U || width == 10U || width == 12U || width == 16U, "DVP bus width must be 8, 10, 12, or 16");
    static_assert(((DataPins >= 0 && DataPins < SOC_GPIO_PIN_COUNT) && ...), "invalid DVP data pin");

    [[nodiscard]] static consteval cam_ctlr_data_width_t data_width() noexcept
    {
        if constexpr (width == 8U) {
            return CAM_CTLR_DATA_WIDTH_8;
        } else if constexpr (width == 10U) {
            return CAM_CTLR_DATA_WIDTH_10;
        } else if constexpr (width == 12U) {
            return CAM_CTLR_DATA_WIDTH_12;
        } else {
            return CAM_CTLR_DATA_WIDTH_16;
        }
    }
};

template <typename DataLines,
          int Vsync,
          int Pclk,
          int De = -1,
          int Xclk = -1,
          std::uint32_t H = 320,
          std::uint32_t V = 240,
          cam_ctlr_color_t In = CAM_CTLR_COLOR_RGB565,
          cam_ctlr_color_t Out = In,
          std::uint32_t XclkHz = 20'000'000,
          std::uint32_t BurstBytes = 64,
          bool ExternalClock = (Xclk == -1),
          bool SwapBytes = false,
          bool SwapBits = false,
          bool Backup = true,
          cam_clock_source_t Source = CAM_CLK_SRC_DEFAULT>
struct Dvp;

template <int... DataPins,
          int Vsync,
          int Pclk,
          int De,
          int Xclk,
          std::uint32_t H,
          std::uint32_t V,
          cam_ctlr_color_t In,
          cam_ctlr_color_t Out,
          std::uint32_t XclkHz,
          std::uint32_t BurstBytes,
          bool ExternalClock,
          bool SwapBytes,
          bool SwapBits,
          bool Backup,
          cam_clock_source_t Source>
struct Dvp<DvpLines<DataPins...>,
           Vsync,
           Pclk,
           De,
           Xclk,
           H,
           V,
           In,
           Out,
           XclkHz,
           BurstBytes,
           ExternalClock,
           SwapBytes,
           SwapBits,
           Backup,
           Source> {
    static_assert(SOC_LCDCAM_CAM_SUPPORTED, "DVP camera is not supported on this target");
    static_assert(Vsync >= 0 && Vsync < SOC_GPIO_PIN_COUNT, "invalid DVP VSYNC pin");
    static_assert(Pclk >= 0 && Pclk < SOC_GPIO_PIN_COUNT, "invalid DVP PCLK pin");
    static_assert((De >= 0 && De < SOC_GPIO_PIN_COUNT) || De == -1, "invalid DVP DE/HREF pin");
    static_assert((Xclk >= 0 && Xclk < SOC_GPIO_PIN_COUNT) || Xclk == -1, "invalid DVP XCLK pin");
    static_assert(H != 0U && V != 0U, "DVP frame size must be non-zero");
    static_assert(BurstBytes == 0U || ((BurstBytes & (BurstBytes - 1U)) == 0U), "DVP DMA burst must be zero or a power of two");
    static_assert(ExternalClock || XclkHz != 0U, "DVP generated XCLK must have a non-zero frequency");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        esp_cam_ctlr_dvp_pin_config_t pins{};
        pins.data_width = DvpLines<DataPins...>::data_width();
        pins.vsync_io = gpio(Vsync);
        pins.de_io = gpio_or_nc(De);
        pins.pclk_io = gpio(Pclk);
        pins.xclk_io = gpio_or_nc(Xclk);

        std::size_t index = 0U;
        ((pins.data_io[index++] = gpio(DataPins)), ...);
        for (; index < ESP_CAM_CTLR_DVP_DATA_SIG_NUM; ++index) {
            pins.data_io[index] = GPIO_NUM_NC;
        }

        esp_cam_ctlr_dvp_config_t config{};
        config.ctlr_id = 0;
        config.clk_src = Source;
        config.h_res = H;
        config.v_res = V;
        config.input_data_color_type = In;
        config.output_data_color_type = Out;
        config.conv_std = COLOR_CONV_STD_RGB_YUV_BT601;
        config.input_range = COLOR_RANGE_FULL;
        config.output_range = COLOR_RANGE_FULL;
        config.cam_data_width = static_cast<std::uint32_t>(DvpLines<DataPins...>::width);
        config.bit_swap_en = SwapBits ? 1U : 0U;
        config.byte_swap_en = SwapBytes ? 1U : 0U;
        config.bk_buffer_dis = Backup ? 0U : 1U;
        config.pin_dont_init = 0U;
        config.pic_format_jpeg = 0U;
        config.external_xtal = ExternalClock ? 1U : 0U;
        config.dma_burst_size = BurstBytes;
        config.xclk_freq = XclkHz;
        config.pin = &pins;
        const auto err = esp_cam_new_dvp_ctlr(&config, &state.cam);
        if (err != ESP_OK) {
            state.cam = nullptr;
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

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        if (!state.enabled) {
            const auto err = esp_cam_ctlr_enable(state.cam);
            if (err != ESP_OK) {
                return err;
            }
            state.enabled = true;
        }
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t run() noexcept
    {
        const auto ready = enable();
        if (ready != ESP_OK) {
            return ready;
        }

        if (!state.running) {
            const auto err = esp_cam_ctlr_start(state.cam);
            if (err != ESP_OK) {
                return err;
            }
            state.running = true;
        }
        return ESP_OK;
    }

    static void start()
    {
        ESP_ERROR_CHECK(run());
    }

    static void stop()
    {
        if (state.cam == nullptr || !state.running) {
            return;
        }

        ESP_ERROR_CHECK(esp_cam_ctlr_stop(state.cam));
        state.running = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        start();
    }

    template <typename T>
    [[nodiscard]] static CapsBuf<T> buffer(
        const std::size_t count,
        const std::uint32_t caps = static_cast<std::uint32_t>(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return {};
        }

        if (count == 0U) {
            return {};
        }
        if (count > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
            return {};
        }

        const auto bytes = count * sizeof(T);
        void* const data = esp_cam_ctlr_alloc_buffer(state.cam, bytes, caps);
        if (data == nullptr) {
            return {};
        }
        return CapsBuf<T>(static_cast<T*>(data), count, bytes);
    }

    template <typename T>
    [[nodiscard]] static esp_err_t grab(
        CapsBuf<T>& frame,
        std::size_t* const got = nullptr,
        const std::uint32_t timeout_ms = 0U) noexcept
    {
        const auto ready = run();
        if (ready != ESP_OK) {
            return ready;
        }

        esp_cam_ctlr_trans_t trans{};
        trans.buffer = frame.data();
        trans.buflen = frame.bytes();
        const auto err = esp_cam_ctlr_receive(state.cam, &trans, timeout_ms);
        if (got != nullptr) {
            *got = trans.received_size;
        }
        if (err == ESP_OK) {
            __atomic_add_fetch(&state.frames, 1U, __ATOMIC_RELEASE);
            __atomic_add_fetch(&state.bytes, trans.received_size, __ATOMIC_RELEASE);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t convert(
        const cam_ctlr_color_t in,
        const cam_ctlr_color_t out,
        const std::uint32_t data_width = static_cast<std::uint32_t>(DvpLines<DataPins...>::width),
        const color_conv_std_rgb_yuv_t standard = COLOR_CONV_STD_RGB_YUV_BT601,
        const color_range_t input = COLOR_RANGE_FULL,
        const color_range_t output = COLOR_RANGE_FULL) noexcept
    {
        const auto ready = init();
        if (ready != ESP_OK) {
            return ready;
        }

        cam_ctlr_format_conv_config_t config{};
        config.src_format = in;
        config.dst_format = out;
        config.conv_std = standard;
        config.data_width = data_width;
        config.input_range = input;
        config.output_range = output;
        return esp_cam_ctlr_format_conversion(state.cam, &config);
    }

    [[nodiscard]] static std::uint32_t frames() noexcept
    {
        return __atomic_load_n(&state.frames, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::size_t bytes() noexcept
    {
        return __atomic_load_n(&state.bytes, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static constexpr std::uint32_t h() noexcept
    {
        return H;
    }

    [[nodiscard]] static constexpr std::uint32_t v() noexcept
    {
        return V;
    }

    [[nodiscard]] static constexpr std::size_t width() noexcept
    {
        return DvpLines<DataPins...>::width;
    }

    [[nodiscard]] static esp_cam_ctlr_handle_t native()
    {
        boot();
        return state.cam;
    }

private:
    struct State {
        esp_cam_ctlr_handle_t cam{};
        alignas(4) std::uint32_t frames{};
        std::size_t bytes{};
        bool enabled{};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static constexpr gpio_num_t gpio(const int pin) noexcept
    {
        return static_cast<gpio_num_t>(pin);
    }

    [[nodiscard]] static constexpr gpio_num_t gpio_or_nc(const int pin) noexcept
    {
        return pin == -1 ? GPIO_NUM_NC : gpio(pin);
    }
};

}  // namespace arc
