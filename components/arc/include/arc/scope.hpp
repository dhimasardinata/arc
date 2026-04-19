#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

#include "arc/fence.hpp"

namespace arc {

template <std::uint32_t Hz,
          std::uint32_t FrameBytes = 256,
          std::uint32_t StoreBytes = 1024,
          bool Flush = true,
          typename... Pads>
struct Scope {
    static_assert(sizeof...(Pads) > 0U, "scope must sample at least one ADC pad");
    static_assert(sizeof...(Pads) <= SOC_ADC_PATT_LEN_MAX, "too many ADC pads for one digital pattern");
    static_assert(Hz != 0U, "ADC sample frequency must be non-zero");
    static_assert(FrameBytes >= SOC_ADC_DIGI_RESULT_BYTES, "ADC frame must hold at least one sample");
    static_assert((FrameBytes % SOC_ADC_DIGI_RESULT_BYTES) == 0U, "ADC frame size must align to sample size");
    static_assert(StoreBytes >= FrameBytes, "ADC store buffer must be at least one frame");

    using Sample = adc_continuous_data_t;

    static void start()
    {
        init();

        if (!state.running) {
            ESP_ERROR_CHECK(adc_continuous_start(state.handle));
            state.running = true;
        }
    }

    static void stop()
    {
        if (!state.running) {
            return;
        }

        ESP_ERROR_CHECK(adc_continuous_stop(state.handle));
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

    static void clear() noexcept
    {
        state.ready = false;
        state.overflow = false;
        state.frames = 0U;
        state.overruns = 0U;
        fence();
    }

    [[nodiscard]] static constexpr std::uint32_t hz() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static constexpr std::size_t pads() noexcept
    {
        return sizeof...(Pads);
    }

    [[nodiscard]] static bool ready() noexcept
    {
        fence();
        return state.ready;
    }

    [[nodiscard]] static bool overflow() noexcept
    {
        fence();
        return state.overflow;
    }

    [[nodiscard]] static std::uint32_t frames() noexcept
    {
        fence();
        return state.frames;
    }

    [[nodiscard]] static std::uint32_t overruns() noexcept
    {
        fence();
        return state.overruns;
    }

    static void flush() noexcept
    {
        init();
        ESP_ERROR_CHECK(adc_continuous_flush_pool(state.handle));
        clear();
    }

    [[nodiscard]] static esp_err_t raw(
        std::uint8_t* data,
        const std::uint32_t capacity,
        std::uint32_t* got,
        const std::uint32_t timeout_ms = 0) noexcept
    {
        init();
        return adc_continuous_read(state.handle, data, capacity, got, timeout_ms);
    }

    [[nodiscard]] static esp_err_t pull(
        Sample* out,
        const std::uint32_t capacity,
        std::uint32_t* got,
        const std::uint32_t timeout_ms = 0) noexcept
    {
        init();
        const auto ret = adc_continuous_read_parse(state.handle, out, capacity, got, timeout_ms);
        if (ret == ESP_OK && got != nullptr && *got != 0U) {
            fence();
            state.ready = false;
        }
        return ret;
    }

private:
    struct State {
        adc_continuous_handle_t handle{};
        volatile bool ready{};
        volatile bool overflow{};
        volatile std::uint32_t frames{};
        volatile std::uint32_t overruns{};
        bool running{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_frame(
        adc_continuous_handle_t,
        const adc_continuous_evt_data_t*,
        void*) noexcept
    {
        state.frames += 1U;
        fence();
        state.ready = true;
        return false;
    }

    IRAM_ATTR static bool on_overflow(
        adc_continuous_handle_t,
        const adc_continuous_evt_data_t*,
        void*) noexcept
    {
        state.overruns += 1U;
        state.overflow = true;
        return false;
    }

    template <typename Pad>
    static void bind(adc_digi_pattern_config_t& slot) noexcept
    {
        adc_unit_t unit{};
        adc_channel_t channel{};
        Pad::resolve(unit, channel);

        slot.atten = static_cast<std::uint8_t>(Pad::atten());
        slot.channel = static_cast<std::uint8_t>(channel);
        slot.unit = static_cast<std::uint8_t>(unit);
        slot.bit_width = static_cast<std::uint8_t>(Pad::width());
    }

    static void init()
    {
        if (state.handle != nullptr) {
            return;
        }

        adc_continuous_handle_cfg_t cfg{};
        cfg.max_store_buf_size = StoreBytes;
        cfg.conv_frame_size = FrameBytes;
        cfg.flags.flush_pool = Flush ? 1U : 0U;
        ESP_ERROR_CHECK(adc_continuous_new_handle(&cfg, &state.handle));

        std::array<adc_digi_pattern_config_t, sizeof...(Pads)> pattern{};
        std::size_t index = 0U;
        (bind<Pads>(pattern[index++]), ...);

        adc_continuous_config_t dig{};
        dig.pattern_num = static_cast<std::uint32_t>(pattern.size());
        dig.adc_pattern = pattern.data();
        dig.sample_freq_hz = Hz;
        dig.conv_mode = ADC_CONV_SINGLE_UNIT_1;
        ESP_ERROR_CHECK(adc_continuous_config(state.handle, &dig));

        adc_continuous_evt_cbs_t cbs{};
        cbs.on_conv_done = &on_frame;
        cbs.on_pool_ovf = &on_overflow;
        ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(state.handle, &cbs, nullptr));
    }
};

}  // namespace arc
