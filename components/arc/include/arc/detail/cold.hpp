#pragma once

#include <cstddef>
#include <cstdint>

#if __has_include("driver/gptimer.h")
#include "driver/gptimer.h"
#endif

#if __has_include("driver/rmt_common.h")
#include "driver/rmt_common.h"
#endif

#if __has_include("driver/mcpwm_prelude.h")
#include "driver/mcpwm_prelude.h"
#endif

#if __has_include("driver/rmt_rx.h")
#include "driver/rmt_rx.h"
#endif

#if __has_include("driver/rmt_tx.h")
#include "driver/rmt_tx.h"
#endif

#if __has_include("driver/rmt_encoder.h")
#include "driver/rmt_encoder.h"
#endif

#if __has_include("driver/temperature_sensor.h")
#include "driver/temperature_sensor.h"
#endif

#if __has_include("driver/usb_serial_jtag.h")
#include "driver/usb_serial_jtag.h"
#endif

#if __has_include("esp_adc/adc_continuous.h")
#include "esp_adc/adc_continuous.h"
#endif

#if __has_include("soc/gpio_num.h")
#include "soc/gpio_num.h"
#endif

namespace arc::detail::cold {

// Share cold driver-configuration bodies across template instantiations.

#if __has_include("driver/gptimer.h")

using TimerAlarmCallback = decltype(gptimer_event_callbacks_t{}.on_alarm);

struct TimerSpec {
    std::uint32_t hz{};
    gptimer_clock_source_t source{GPTIMER_CLK_SRC_DEFAULT};
    gptimer_count_direction_t direction{GPTIMER_COUNT_UP};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t timer_create(
    const TimerSpec spec,
    gptimer_handle_t* const out) noexcept
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    gptimer_config_t config{};
    config.clk_src = spec.source;
    config.direction = spec.direction;
    config.resolution_hz = spec.hz;
    config.intr_priority = 0;
    config.flags.intr_shared = 0;
    config.flags.allow_pd = 0;
    return gptimer_new_timer(&config, out);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t timer_bind(
    const gptimer_handle_t timer,
    const TimerAlarmCallback callback,
    void* const user = nullptr) noexcept
{
    if (timer == nullptr || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    gptimer_event_callbacks_t callbacks{};
    callbacks.on_alarm = callback;
    return gptimer_register_event_callbacks(timer, &callbacks, user);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t timer_alarm(
    const gptimer_handle_t timer,
    const std::uint64_t count,
    const std::uint64_t reload = 0,
    const bool auto_reload = false) noexcept
{
    if (timer == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    gptimer_alarm_config_t config{};
    config.alarm_count = count;
    config.reload_count = reload;
    config.flags.auto_reload_on_alarm = auto_reload ? 1U : 0U;
    return gptimer_set_alarm_action(timer, &config);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t timer_alarm_off(
    const gptimer_handle_t timer) noexcept
{
    if (timer == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    return gptimer_set_alarm_action(timer, nullptr);
}

#endif

#if __has_include("driver/rmt_common.h")

[[gnu::cold, gnu::noinline]] inline esp_err_t rmt_carrier(
    const rmt_channel_handle_t channel,
    const std::uint32_t frequency_hz,
    const float duty_cycle = 50.0f,
    const bool active_low = false,
    const bool always_on = false) noexcept
{
    if (channel == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_carrier_config_t config{};
    config.frequency_hz = frequency_hz;
    config.duty_cycle = duty_cycle;
    config.flags.polarity_active_low = active_low ? 1U : 0U;
    config.flags.always_on = always_on ? 1U : 0U;
    return rmt_apply_carrier(channel, &config);
}

#endif

#if __has_include("driver/rmt_tx.h") && __has_include("driver/rmt_encoder.h") && __has_include("soc/gpio_num.h")

struct BurstSpec {
    int pin{};
    std::uint32_t hz{};
    std::size_t symbols{};
    std::size_t depth{};
    bool dma{};
    rmt_clock_source_t source{RMT_CLK_SRC_DEFAULT};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t burst_create(
    const BurstSpec spec,
    rmt_channel_handle_t* const channel,
    rmt_encoder_handle_t* const encoder) noexcept
{
    if (channel == nullptr || encoder == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_tx_channel_config_t channel_cfg{};
    channel_cfg.gpio_num = static_cast<gpio_num_t>(spec.pin);
    channel_cfg.clk_src = spec.source;
    channel_cfg.resolution_hz = spec.hz;
    channel_cfg.mem_block_symbols = spec.symbols;
    channel_cfg.trans_queue_depth = spec.depth;
    channel_cfg.intr_priority = 0;
    channel_cfg.flags.invert_out = 0;
    channel_cfg.flags.with_dma = spec.dma ? 1U : 0U;
    channel_cfg.flags.allow_pd = 0;
    channel_cfg.flags.init_level = 0;
    auto err = rmt_new_tx_channel(&channel_cfg, channel);
    if (err != ESP_OK) {
        return err;
    }

    rmt_copy_encoder_config_t encoder_cfg{};
    return rmt_new_copy_encoder(&encoder_cfg, encoder);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t burst_transmit(
    const rmt_channel_handle_t channel,
    const rmt_encoder_handle_t encoder,
    const rmt_symbol_word_t* const symbols,
    const std::size_t count,
    const int loop = 0,
    const bool nonblocking = true,
    const bool eot = false) noexcept
{
    if (channel == nullptr || encoder == nullptr || symbols == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_transmit_config_t config{};
    config.loop_count = loop;
    config.flags.eot_level = eot ? 1U : 0U;
    config.flags.queue_nonblocking = nonblocking ? 1U : 0U;
    return rmt_transmit(channel, encoder, symbols, count * sizeof(rmt_symbol_word_t), &config);
}

#endif

#if __has_include("driver/rmt_rx.h") && __has_include("soc/gpio_num.h")

using TraceDoneCallback = decltype(rmt_rx_event_callbacks_t{}.on_recv_done);

struct TraceSpec {
    int pin{};
    std::uint32_t hz{};
    std::size_t symbols{};
    bool dma{};
    rmt_clock_source_t source{RMT_CLK_SRC_DEFAULT};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t trace_create(
    const TraceSpec spec,
    rmt_channel_handle_t* const out) noexcept
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_rx_channel_config_t channel{};
    channel.gpio_num = static_cast<gpio_num_t>(spec.pin);
    channel.clk_src = spec.source;
    channel.resolution_hz = spec.hz;
    channel.mem_block_symbols = spec.symbols;
    channel.intr_priority = 0;
    channel.flags.invert_in = 0;
    channel.flags.with_dma = spec.dma ? 1U : 0U;
    channel.flags.allow_pd = 0;
    return rmt_new_rx_channel(&channel, out);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t trace_bind(
    const rmt_channel_handle_t channel,
    const TraceDoneCallback callback,
    void* const user = nullptr) noexcept
{
    if (channel == nullptr || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_rx_event_callbacks_t callbacks{};
    callbacks.on_recv_done = callback;
    return rmt_rx_register_event_callbacks(channel, &callbacks, user);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t trace_receive(
    const rmt_channel_handle_t channel,
    rmt_symbol_word_t* const data,
    const std::size_t bytes,
    const std::uint32_t min_ns,
    const std::uint32_t max_ns,
    const bool partial = false) noexcept
{
    if (channel == nullptr || data == nullptr || bytes == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    rmt_receive_config_t config{};
    config.signal_range_min_ns = min_ns;
    config.signal_range_max_ns = max_ns;
    config.flags.en_partial_rx = partial ? 1U : 0U;
    return rmt_receive(channel, data, bytes, &config);
}

#endif

#if __has_include("driver/temperature_sensor.h")

struct TempSpec {
    int min{};
    int max{};
    bool allow_pd{};
    temperature_sensor_clk_src_t source{TEMPERATURE_SENSOR_CLK_SRC_DEFAULT};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t temp_install(
    const TempSpec spec,
    temperature_sensor_handle_t* const out) noexcept
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    temperature_sensor_config_t config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(spec.min, spec.max);
    config.clk_src = spec.source;
    config.flags.allow_pd = spec.allow_pd ? 1U : 0U;
    return temperature_sensor_install(&config, out);
}

#endif

#if __has_include("driver/usb_serial_jtag.h")

struct UsbSpec {
    std::uint32_t tx{};
    std::uint32_t rx{};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t usb_install(const UsbSpec spec) noexcept
{
    usb_serial_jtag_driver_config_t config{};
    config.tx_buffer_size = spec.tx;
    config.rx_buffer_size = spec.rx;
    return usb_serial_jtag_driver_install(&config);
}

#endif

#if __has_include("driver/mcpwm_prelude.h")

using CaptureEdgeCallback = decltype(mcpwm_capture_event_callbacks_t{}.on_cap);

struct CaptureSpec {
    int pin{};
    std::uint32_t hz{};
    int group{};
    std::uint32_t prescale{};
    bool rise{};
    bool fall{};
    bool invert{};
    mcpwm_capture_clock_source_t source{MCPWM_CAPTURE_CLK_SRC_DEFAULT};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t capture_create(
    const CaptureSpec spec,
    mcpwm_cap_timer_handle_t* const timer,
    mcpwm_cap_channel_handle_t* const channel) noexcept
{
    if (timer == nullptr || channel == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    mcpwm_capture_timer_config_t timer_cfg{};
    timer_cfg.group_id = spec.group;
    timer_cfg.clk_src = spec.source;
    timer_cfg.resolution_hz = spec.hz;
    timer_cfg.flags.allow_pd = 0U;
    auto err = mcpwm_new_capture_timer(&timer_cfg, timer);
    if (err != ESP_OK) {
        return err;
    }

    mcpwm_capture_channel_config_t channel_cfg{};
    channel_cfg.gpio_num = spec.pin;
    channel_cfg.intr_priority = 0;
    channel_cfg.prescale = spec.prescale;
    channel_cfg.flags.pos_edge = spec.rise ? 1U : 0U;
    channel_cfg.flags.neg_edge = spec.fall ? 1U : 0U;
    channel_cfg.flags.invert_cap_signal = spec.invert ? 1U : 0U;
    return mcpwm_new_capture_channel(*timer, &channel_cfg, channel);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t capture_bind(
    const mcpwm_cap_channel_handle_t channel,
    const CaptureEdgeCallback callback,
    void* const user = nullptr) noexcept
{
    if (channel == nullptr || callback == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    mcpwm_capture_event_callbacks_t callbacks{};
    callbacks.on_cap = callback;
    return mcpwm_capture_channel_register_event_callbacks(channel, &callbacks, user);
}

#endif

#if __has_include("esp_adc/adc_continuous.h")

using ScopeFrameCallback = decltype(adc_continuous_evt_cbs_t{}.on_conv_done);
using ScopeOverflowCallback = decltype(adc_continuous_evt_cbs_t{}.on_pool_ovf);

struct ScopeSpec {
    std::uint32_t frame_bytes{};
    std::uint32_t store_bytes{};
    bool flush{};
};

[[gnu::cold, gnu::noinline]] inline esp_err_t scope_create(
    const ScopeSpec spec,
    adc_continuous_handle_t* const out) noexcept
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_continuous_handle_cfg_t config{};
    config.max_store_buf_size = spec.store_bytes;
    config.conv_frame_size = spec.frame_bytes;
    config.flags.flush_pool = spec.flush ? 1U : 0U;
    return adc_continuous_new_handle(&config, out);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t scope_config(
    const adc_continuous_handle_t handle,
    adc_digi_pattern_config_t* const pattern,
    const std::size_t count,
    const std::uint32_t hz) noexcept
{
    if (handle == nullptr || pattern == nullptr || count == 0U || hz == 0U) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_continuous_config_t config{};
    config.pattern_num = static_cast<std::uint32_t>(count);
    config.adc_pattern = pattern;
    config.sample_freq_hz = hz;
    config.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    return adc_continuous_config(handle, &config);
}

[[gnu::cold, gnu::noinline]] inline esp_err_t scope_bind(
    const adc_continuous_handle_t handle,
    const ScopeFrameCallback on_frame,
    const ScopeOverflowCallback on_overflow,
    void* const user = nullptr) noexcept
{
    if (handle == nullptr || on_frame == nullptr || on_overflow == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    adc_continuous_evt_cbs_t callbacks{};
    callbacks.on_conv_done = on_frame;
    callbacks.on_pool_ovf = on_overflow;
    return adc_continuous_register_event_callbacks(handle, &callbacks, user);
}

#endif

}  // namespace arc::detail::cold
