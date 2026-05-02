#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/cache.hpp"
#include "arc/caps.hpp"
#include "arc/claim.hpp"
#include "arc/init.hpp"
#include "arc/sdk.hpp"

namespace arc {

template <int... DataPins>
struct RgbLines {
    static constexpr std::size_t width = sizeof...(DataPins);
    static_assert(width > 0U, "RGB bus width must be non-zero");
    static_assert(width <= ESP_LCD_RGB_BUS_WIDTH_MAX, "RGB bus width exceeds ESP-LCD maximum");
    static_assert((width % 8U) == 0U, "RGB bus width must be a multiple of 8");
    static_assert(((DataPins >= 0 && DataPins < SOC_GPIO_PIN_COUNT) && ...), "invalid RGB data pin");
};

template <typename DataLines,
          int Hsync = -1,
          int Vsync = -1,
          int De = -1,
          int Pclk = -1,
          int Disp = -1,
          std::uint32_t H = 800,
          std::uint32_t V = 480,
          std::uint32_t PclkHz = 16'000'000,
          std::uint32_t HsyncPulse = 4,
          std::uint32_t HsyncBack = 8,
          std::uint32_t HsyncFront = 8,
          std::uint32_t VsyncPulse = 4,
          std::uint32_t VsyncBack = 8,
          std::uint32_t VsyncFront = 8,
          lcd_color_format_t In = static_cast<lcd_color_format_t>(0),
          lcd_color_format_t Out = static_cast<lcd_color_format_t>(0),
          std::size_t FrameBuffers = 1,
          std::size_t BouncePixels = 0,
          std::size_t BurstBytes = 64,
          bool RefreshOnDemand = false,
          bool FbInPsram = true,
          bool DispActiveLow = false,
          bool HsyncIdleLow = false,
          bool VsyncIdleLow = false,
          bool DeIdleHigh = false,
          bool PclkActiveNeg = false,
          bool PclkIdleHigh = false,
          bool BbInvalidateCache = false,
          lcd_clock_source_t Source = LCD_CLK_SRC_DEFAULT>
struct Rgb;

template <int... DataPins,
          int Hsync,
          int Vsync,
          int De,
          int Pclk,
          int Disp,
          std::uint32_t H,
          std::uint32_t V,
          std::uint32_t PclkHz,
          std::uint32_t HsyncPulse,
          std::uint32_t HsyncBack,
          std::uint32_t HsyncFront,
          std::uint32_t VsyncPulse,
          std::uint32_t VsyncBack,
          std::uint32_t VsyncFront,
          lcd_color_format_t In,
          lcd_color_format_t Out,
          std::size_t FrameBuffers,
          std::size_t BouncePixels,
          std::size_t BurstBytes,
          bool RefreshOnDemand,
          bool FbInPsram,
          bool DispActiveLow,
          bool HsyncIdleLow,
          bool VsyncIdleLow,
          bool DeIdleHigh,
          bool PclkActiveNeg,
          bool PclkIdleHigh,
          bool BbInvalidateCache,
          lcd_clock_source_t Source>
struct Rgb<RgbLines<DataPins...>,
           Hsync,
           Vsync,
           De,
           Pclk,
           Disp,
           H,
           V,
           PclkHz,
           HsyncPulse,
           HsyncBack,
           HsyncFront,
           VsyncPulse,
           VsyncBack,
           VsyncFront,
           In,
           Out,
           FrameBuffers,
           BouncePixels,
           BurstBytes,
           RefreshOnDemand,
           FbInPsram,
           DispActiveLow,
           HsyncIdleLow,
           VsyncIdleLow,
           DeIdleHigh,
           PclkActiveNeg,
           PclkIdleHigh,
           BbInvalidateCache,
           Source> {
    static_assert(SOC_LCD_RGB_SUPPORTED, "RGB LCD is not supported on this target");
    static_assert((Hsync >= 0 && Hsync < SOC_GPIO_PIN_COUNT) || Hsync == -1, "invalid RGB HSYNC pin");
    static_assert((Vsync >= 0 && Vsync < SOC_GPIO_PIN_COUNT) || Vsync == -1, "invalid RGB VSYNC pin");
    static_assert((De >= 0 && De < SOC_GPIO_PIN_COUNT) || De == -1, "invalid RGB DE pin");
    static_assert((Pclk >= 0 && Pclk < SOC_GPIO_PIN_COUNT) || Pclk == -1, "invalid RGB PCLK pin");
    static_assert((Disp >= 0 && Disp < SOC_GPIO_PIN_COUNT) || Disp == -1, "invalid RGB DISP pin");
    static_assert(H != 0U && V != 0U, "RGB resolution must be non-zero");
    static_assert(PclkHz != 0U, "RGB pixel clock must be non-zero");
    static_assert(FrameBuffers > 0U && FrameBuffers <= ESP_RGB_LCD_PANEL_MAX_FB_NUM, "RGB frame buffer count must be 1..3");
    static_assert(BurstBytes > 0U, "RGB DMA burst size must be non-zero");
    static_assert(Hsync != -1 || Vsync != -1 || De != -1, "RGB output requires DE or sync signals");
    static_assert(RgbLines<DataPins...>::width != 8U || In != static_cast<lcd_color_format_t>(0),
                  "8-bit RGB requires an explicit input color format");

    [[nodiscard]] static esp_err_t init(
        void* const fb0 = nullptr,
        void* const fb1 = nullptr,
        void* const fb2 = nullptr) noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        const auto user_count = count_user_fbs(fb0, fb1, fb2);
        if (user_count != 0U && user_count != FrameBuffers) {
            Resource::drop();
            Init::fail(state.init);
            return ESP_ERR_INVALID_ARG;
        }

        auto cfg = config(fb0, fb1, fb2);
        err = esp_lcd_new_rgb_panel(&cfg, &state.panel);
        if (err != ESP_OK) {
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        err = bind();
        if (err != ESP_OK) {
            clear();
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        err = esp_lcd_panel_reset(state.panel);
        if (err != ESP_OK) {
            clear();
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        err = esp_lcd_panel_init(state.panel);
        if (err != ESP_OK) {
            clear();
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        state.pclk = PclkHz;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    static void boot(
        void* const fb0,
        void* const fb1 = nullptr,
        void* const fb2 = nullptr)
    {
        ESP_ERROR_CHECK(init(fb0, fb1, fb2));
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = esp_lcd_panel_del(state.panel);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        clear_counters();
        state.panel = nullptr;
        state.pclk = 0U;
        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t draw(
        const int x0,
        const int y0,
        const int x1,
        const int y1,
        const void* const data) noexcept
    {
        boot();
        const auto err = esp_lcd_panel_draw_bitmap(state.panel, x0, y0, x1, y1, data);
        if (err == ESP_OK) {
            const auto w = x1 > x0 ? static_cast<std::uint32_t>(x1 - x0) : 0U;
            const auto h = y1 > y0 ? static_cast<std::uint32_t>(y1 - y0) : 0U;
            __atomic_add_fetch(&state.pixels, static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h), __ATOMIC_RELEASE);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t draw_coherent(
        const int x0,
        const int y0,
        const int x1,
        const int y1,
        void* const data,
        const std::size_t bytes) noexcept
    {
        const auto err = Cache::to_device(data, bytes);
        if (err != ESP_OK) {
            return err;
        }
        return draw(x0, y0, x1, y1, data);
    }

    [[nodiscard]] static esp_err_t draw_2d(
        const int x0,
        const int y0,
        const int x1,
        const int y1,
        const void* const data,
        const std::size_t src_x,
        const std::size_t src_y,
        const int src_x0,
        const int src_y0,
        const int src_x1,
        const int src_y1) noexcept
    {
        boot();
        const auto err = esp_lcd_panel_draw_bitmap_2d(
            state.panel,
            x0,
            y0,
            x1,
            y1,
            data,
            src_x,
            src_y,
            src_x0,
            src_y0,
            src_x1,
            src_y1);
        if (err == ESP_OK) {
            const auto w = x1 > x0 ? static_cast<std::uint32_t>(x1 - x0) : 0U;
            const auto h = y1 > y0 ? static_cast<std::uint32_t>(y1 - y0) : 0U;
            __atomic_add_fetch(&state.pixels, static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h), __ATOMIC_RELEASE);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t draw_2d_coherent(
        const int x0,
        const int y0,
        const int x1,
        const int y1,
        void* const data,
        const std::size_t bytes,
        const std::size_t src_x,
        const std::size_t src_y,
        const int src_x0,
        const int src_y0,
        const int src_x1,
        const int src_y1) noexcept
    {
        const auto err = Cache::to_device(data, bytes);
        if (err != ESP_OK) {
            return err;
        }
        return draw_2d(x0, y0, x1, y1, data, src_x, src_y, src_x0, src_y0, src_x1, src_y1);
    }

    [[nodiscard]] static esp_err_t frame(const void* const data) noexcept
    {
        return draw(0, 0, static_cast<int>(H), static_cast<int>(V), data);
    }

    [[nodiscard]] static esp_err_t frame_coherent(
        void* const data,
        const std::size_t bytes) noexcept
    {
        return draw_coherent(0, 0, static_cast<int>(H), static_cast<int>(V), data, bytes);
    }

    [[nodiscard]] static esp_err_t refresh() noexcept
    {
        boot();
        return esp_lcd_rgb_panel_refresh(state.panel);
    }

    [[nodiscard]] static esp_err_t restart() noexcept
    {
        boot();
        return esp_lcd_rgb_panel_restart(state.panel);
    }

    [[nodiscard]] static esp_err_t pclk(const std::uint32_t value) noexcept
    {
        boot();
        const auto err = esp_lcd_rgb_panel_set_pclk(state.panel, value);
        if (err == ESP_OK) {
            state.pclk = value;
        }
        return err;
    }

    [[nodiscard]] static esp_err_t disp(const bool on) noexcept
    {
        boot();
        return esp_lcd_panel_disp_on_off(state.panel, on);
    }

    [[nodiscard]] static esp_err_t sleep(const bool enable) noexcept
    {
        boot();
        return esp_lcd_panel_disp_sleep(state.panel, enable);
    }

    [[nodiscard]] static esp_err_t mirror(const bool x, const bool y) noexcept
    {
        boot();
        return esp_lcd_panel_mirror(state.panel, x, y);
    }

    [[nodiscard]] static esp_err_t swap(const bool xy) noexcept
    {
        boot();
        return esp_lcd_panel_swap_xy(state.panel, xy);
    }

    [[nodiscard]] static esp_err_t gap(const int x, const int y) noexcept
    {
        boot();
        return esp_lcd_panel_set_gap(state.panel, x, y);
    }

    [[nodiscard]] static esp_err_t invert(const bool enable) noexcept
    {
        boot();
        return esp_lcd_panel_invert_color(state.panel, enable);
    }

    [[nodiscard]] static esp_err_t yuv(const esp_lcd_color_conv_yuv_config_t& cfg) noexcept
    {
        boot();
        return esp_lcd_rgb_panel_set_yuv_conversion(state.panel, &cfg);
    }

    template <typename T = std::byte>
    [[nodiscard]] static T* fb(const std::size_t index = 0U) noexcept
    {
        return static_cast<T*>(fb_raw(index));
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

        void* const data = esp_lcd_rgb_alloc_draw_buffer(state.panel, count * sizeof(T), caps);
        return {static_cast<T*>(data), data == nullptr ? 0U : count};
    }

    [[nodiscard]] static std::uint32_t draws() noexcept
    {
        return __atomic_load_n(&state.draws, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t frame_done() noexcept
    {
        return __atomic_load_n(&state.frame_done, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint32_t vsyncs() noexcept
    {
        return __atomic_load_n(&state.vsyncs, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static std::uint64_t pixels() noexcept
    {
        return __atomic_load_n(&state.pixels, __ATOMIC_ACQUIRE);
    }

    [[nodiscard]] static constexpr std::size_t width() noexcept
    {
        return RgbLines<DataPins...>::width;
    }

    [[nodiscard]] static constexpr std::uint32_t h() noexcept
    {
        return H;
    }

    [[nodiscard]] static constexpr std::uint32_t v() noexcept
    {
        return V;
    }

    [[nodiscard]] static constexpr std::size_t fbs() noexcept
    {
        return FrameBuffers;
    }

    [[nodiscard]] static constexpr std::size_t bounce() noexcept
    {
        return BouncePixels;
    }

    [[nodiscard]] static std::uint32_t pclk() noexcept
    {
        boot();
        return state.pclk;
    }

    [[nodiscard]] static constexpr bool demand() noexcept
    {
        return RefreshOnDemand;
    }

    [[nodiscard]] static constexpr bool psram() noexcept
    {
        return FbInPsram;
    }

    [[nodiscard]] static constexpr lcd_color_format_t in() noexcept
    {
        return In;
    }

    [[nodiscard]] static constexpr lcd_color_format_t out() noexcept
    {
        return Out;
    }

    [[nodiscard]] static esp_lcd_panel_handle_t native()
    {
        boot();
        return state.panel;
    }

private:
    using Resource = ClaimFor<ClaimKind::lcd_rgb,
                              0,
                              DataPins...,
                              Hsync,
                              Vsync,
                              De,
                              Pclk,
                              Disp,
                              H,
                              V,
                              PclkHz,
                              HsyncPulse,
                              HsyncBack,
                              HsyncFront,
                              VsyncPulse,
                              VsyncBack,
                              VsyncFront,
                              In,
                              Out,
                              FrameBuffers,
                              BouncePixels,
                              BurstBytes,
                              RefreshOnDemand,
                              FbInPsram,
                              DispActiveLow,
                              HsyncIdleLow,
                              VsyncIdleLow,
                              DeIdleHigh,
                              PclkActiveNeg,
                              PclkIdleHigh,
                              BbInvalidateCache,
                              Source>;

    struct State {
        esp_lcd_panel_handle_t panel{};
        std::uint32_t init{};
        std::uint32_t pclk{};
        alignas(cache_line) std::uint32_t draws{};
        alignas(cache_line) std::uint32_t frame_done{};
        alignas(cache_line) std::uint32_t vsyncs{};
        alignas(cache_line) std::uint64_t pixels{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_draw(
        esp_lcd_panel_handle_t,
        const esp_lcd_rgb_panel_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.draws, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_frame(
        esp_lcd_panel_handle_t,
        const esp_lcd_rgb_panel_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.frame_done, 1U, __ATOMIC_RELEASE);
        return false;
    }

    IRAM_ATTR static bool on_vsync(
        esp_lcd_panel_handle_t,
        const esp_lcd_rgb_panel_event_data_t*,
        void*) noexcept
    {
        __atomic_add_fetch(&state.vsyncs, 1U, __ATOMIC_RELEASE);
        return false;
    }

    [[nodiscard]] static constexpr gpio_num_t gpio(const int pin) noexcept
    {
        return static_cast<gpio_num_t>(pin);
    }

    [[nodiscard]] static constexpr std::size_t count_user_fbs(
        void* const fb0,
        void* const fb1,
        void* const fb2) noexcept
    {
        return (fb0 != nullptr ? 1U : 0U) + (fb1 != nullptr ? 1U : 0U) + (fb2 != nullptr ? 1U : 0U);
    }

    [[nodiscard]] static constexpr esp_lcd_rgb_timing_t timing() noexcept
    {
        esp_lcd_rgb_timing_t cfg{};
        cfg.pclk_hz = PclkHz;
        cfg.h_res = H;
        cfg.v_res = V;
        cfg.hsync_pulse_width = HsyncPulse;
        cfg.hsync_back_porch = HsyncBack;
        cfg.hsync_front_porch = HsyncFront;
        cfg.vsync_pulse_width = VsyncPulse;
        cfg.vsync_back_porch = VsyncBack;
        cfg.vsync_front_porch = VsyncFront;
        cfg.flags.hsync_idle_low = HsyncIdleLow ? 1U : 0U;
        cfg.flags.vsync_idle_low = VsyncIdleLow ? 1U : 0U;
        cfg.flags.de_idle_high = DeIdleHigh ? 1U : 0U;
        cfg.flags.pclk_active_neg = PclkActiveNeg ? 1U : 0U;
        cfg.flags.pclk_idle_high = PclkIdleHigh ? 1U : 0U;
        return cfg;
    }

    [[nodiscard]] static constexpr esp_lcd_rgb_panel_config_t config(
        void* const fb0,
        void* const fb1,
        void* const fb2) noexcept
    {
        esp_lcd_rgb_panel_config_t cfg{};
        cfg.clk_src = Source;
        cfg.timings = timing();
        cfg.data_width = RgbLines<DataPins...>::width;
        cfg.in_color_format = In;
        cfg.out_color_format = Out;
        cfg.num_fbs = FrameBuffers;
        cfg.user_fbs[0] = fb0;
        cfg.user_fbs[1] = fb1;
        cfg.user_fbs[2] = fb2;
        cfg.bounce_buffer_size_px = BouncePixels;
        cfg.dma_burst_size = BurstBytes;
        cfg.hsync_gpio_num = gpio(Hsync);
        cfg.vsync_gpio_num = gpio(Vsync);
        cfg.de_gpio_num = gpio(De);
        cfg.pclk_gpio_num = gpio(Pclk);
        cfg.disp_gpio_num = gpio(Disp);
        std::size_t index = 0U;
        ((cfg.data_gpio_nums[index++] = gpio(DataPins)), ...);
        for (; index < ESP_LCD_RGB_BUS_WIDTH_MAX; ++index) {
            cfg.data_gpio_nums[index] = GPIO_NUM_NC;
        }
        cfg.flags.disp_active_low = DispActiveLow ? 1U : 0U;
        cfg.flags.refresh_on_demand = RefreshOnDemand ? 1U : 0U;
        cfg.flags.fb_in_psram = FbInPsram ? 1U : 0U;
        cfg.flags.double_fb = 0U;
        cfg.flags.no_fb = 0U;
        cfg.flags.bb_invalidate_cache = BbInvalidateCache ? 1U : 0U;
        return cfg;
    }

    [[nodiscard]] static esp_err_t bind() noexcept
    {
        esp_lcd_rgb_panel_event_callbacks_t cb{};
        cb.on_color_trans_done = &on_draw;
        cb.on_frame_buf_complete = &on_frame;
        cb.on_vsync = &on_vsync;
        return esp_lcd_rgb_panel_register_event_callbacks(state.panel, &cb, nullptr);
    }

    [[nodiscard]] static void* fb_raw(const std::size_t index) noexcept
    {
        boot();
        if (index >= FrameBuffers) {
            return nullptr;
        }

        void* fb0{};
        void* fb1{};
        void* fb2{};
        esp_err_t err{};
        if constexpr (FrameBuffers == 1U) {
            err = esp_lcd_rgb_panel_get_frame_buffer(state.panel, 1, &fb0);
        } else if constexpr (FrameBuffers == 2U) {
            err = esp_lcd_rgb_panel_get_frame_buffer(state.panel, 2, &fb0, &fb1);
        } else {
            err = esp_lcd_rgb_panel_get_frame_buffer(state.panel, 3, &fb0, &fb1, &fb2);
        }
        if (err != ESP_OK) {
            return nullptr;
        }

        if (index == 0U) {
            return fb0;
        }
        if (index == 1U) {
            return fb1;
        }
        return fb2;
    }

    static void clear() noexcept
    {
        if (state.panel != nullptr) {
            (void)esp_lcd_panel_del(state.panel);
            state.panel = nullptr;
        }
    }

    static void clear_counters() noexcept
    {
        __atomic_store_n(&state.draws, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.frame_done, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.vsyncs, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&state.pixels, 0U, __ATOMIC_RELEASE);
    }
};

}  // namespace arc
