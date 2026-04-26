#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"

#include "arc/detail/cold.hpp"
#include "arc/fence.hpp"
#include "arc/init.hpp"

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
        state.ready.store(false, std::memory_order_release);
        state.overflow.store(false, std::memory_order_relaxed);
        state.frames.store(0U, std::memory_order_relaxed);
        state.overruns.store(0U, std::memory_order_relaxed);
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
        return state.ready.load(std::memory_order_acquire);
    }

    [[nodiscard]] static bool overflow() noexcept
    {
        return state.overflow.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t frames() noexcept
    {
        return state.frames.load(std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint32_t overruns() noexcept
    {
        return state.overruns.load(std::memory_order_relaxed);
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
        const std::uint32_t samples,
        std::uint32_t* got,
        const std::uint32_t timeout_ms = 0) noexcept
    {
        init();
        const auto ret = adc_continuous_read_parse(state.handle, out, samples, got, timeout_ms);
        if (ret == ESP_OK && got != nullptr && *got != 0U) {
            state.ready.store(false, std::memory_order_release);
        }
        return ret;
    }

private:
    struct State {
        adc_continuous_handle_t handle{};
        std::atomic<bool> ready{false};
        std::atomic<bool> overflow{false};
        std::atomic<std::uint32_t> frames{0U};
        std::atomic<std::uint32_t> overruns{0U};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    IRAM_ATTR static bool on_frame(
        adc_continuous_handle_t,
        const adc_continuous_evt_data_t*,
        void*) noexcept
    {
        state.frames.fetch_add(1U, std::memory_order_relaxed);
        fence();
        state.ready.store(true, std::memory_order_release);
        return false;
    }

    IRAM_ATTR static bool on_overflow(
        adc_continuous_handle_t,
        const adc_continuous_evt_data_t*,
        void*) noexcept
    {
        state.overruns.fetch_add(1U, std::memory_order_relaxed);
        state.overflow.store(true, std::memory_order_release);
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

    [[gnu::cold]] static void init()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        ESP_ERROR_CHECK(detail::cold::scope_create({FrameBytes, StoreBytes, Flush}, &state.handle));

        std::array<adc_digi_pattern_config_t, sizeof...(Pads)> pattern{};
        std::size_t index = 0U;
        (bind<Pads>(pattern[index++]), ...);

        ESP_ERROR_CHECK(detail::cold::scope_config(state.handle, pattern.data(), pattern.size(), Hz));
        ESP_ERROR_CHECK(detail::cold::scope_bind(state.handle, &on_frame, &on_overflow));
        Init::pass(state.init);
    }
};

}  // namespace arc
