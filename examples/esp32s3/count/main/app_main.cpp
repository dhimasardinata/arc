#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace app {

inline constexpr char tag[] = "arc-count";
inline constexpr std::uint32_t stack = CONFIG_ARC_COUNT_STACK;
inline constexpr TickType_t sample_ticks = pdMS_TO_TICKS(CONFIG_ARC_COUNT_SAMPLE_MS);
inline constexpr std::uint16_t half_us = static_cast<std::uint16_t>(CONFIG_ARC_COUNT_HALF_US);

using Source = arc::Burst<CONFIG_ARC_COUNT_OUT, 1'000'000, 64, 1, false>;
using Meter = arc::Count<
    CONFIG_ARC_COUNT_IN,
    -1,
    -1,
    -32768,
    32767,
    PCNT_CHANNEL_EDGE_ACTION_INCREASE,
    PCNT_CHANNEL_EDGE_ACTION_HOLD,
    PCNT_CHANNEL_LEVEL_ACTION_KEEP,
    PCNT_CHANNEL_LEVEL_ACTION_KEEP,
    static_cast<std::uint32_t>(CONFIG_ARC_COUNT_FILTER_NS)>;

constinit static arc::TaskMem<stack> host_mem{};

constinit static rmt_symbol_word_t frame[] = {
    Source::symbol(half_us, true, half_us, false),
    Source::symbol(half_us, true, half_us, false),
    Source::symbol(half_us, true, half_us, false),
    Source::symbol(half_us, true, half_us, false),
};

void host(void*) noexcept
{
    int last = Meter::read();

    while (true) {
        vTaskDelay(sample_ticks);

        const auto now = Meter::read();
        const auto delta = now - last;
        last = now;

        ESP_LOGI(
            tag,
            "count=%d delta=%d pulses/%u ms out=%d in=%d half=%u us",
            now,
            delta,
            static_cast<unsigned>(CONFIG_ARC_COUNT_SAMPLE_MS),
            CONFIG_ARC_COUNT_OUT,
            CONFIG_ARC_COUNT_IN,
            static_cast<unsigned>(half_us));
    }
}

inline void boot()
{
    Meter::start();
    Source::start();
    ESP_ERROR_CHECK(Source::send(frame, -1, true));

    const auto handle = arc::spawn(
        &host,
        "count",
        nullptr,
        1,
        arc::Core::core0,
        host_mem);
    configASSERT(handle != nullptr);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
