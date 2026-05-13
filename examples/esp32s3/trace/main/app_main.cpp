#include <array>
#include <cstddef>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace app {

inline constexpr char tag[] = "arc-trace";
inline constexpr std::uint32_t stack = CONFIG_ARC_TRACE_STACK;
inline constexpr TickType_t sample_ticks = pdMS_TO_TICKS(CONFIG_ARC_TRACE_SAMPLE_MS);
inline constexpr std::uint16_t half = static_cast<std::uint16_t>(CONFIG_ARC_TRACE_HALF_US);

using Source = arc::Burst<CONFIG_ARC_TRACE_OUT, 1'000'000, 64, 1, false>;
using Sink = arc::Trace<
    CONFIG_ARC_TRACE_IN,
    1'000'000,
    64,
    static_cast<std::uint32_t>(CONFIG_ARC_TRACE_MIN_NS),
    static_cast<std::uint32_t>(CONFIG_ARC_TRACE_MAX_NS),
    false>;
using View = Sink::View;

constinit static arc::TaskMem<stack> host_mem{};

constinit static std::array<rmt_symbol_word_t, 4> frame{
    Source::symbol(half, true, half, false),
    Source::symbol(static_cast<std::uint16_t>(half * 2U), true, half, false),
    Source::symbol(static_cast<std::uint16_t>(half * 3U), true, half, false),
    Source::symbol(static_cast<std::uint16_t>(half * 4U), true, half, false),
};

static void dump(const View symbols) noexcept
{
    if (symbols.empty()) {
        ESP_LOGW(tag, "capture empty");
        return;
    }

    ESP_LOGI(
        tag,
        "capture n=%u out=%d in=%d last=%d",
        static_cast<unsigned>(symbols.size()),
        CONFIG_ARC_TRACE_OUT,
        CONFIG_ARC_TRACE_IN,
        Sink::last() ? 1 : 0);

    for (std::size_t i = 0; i < symbols.size(); ++i) {
        ESP_LOGI(
            tag,
            "  [%u] %d:%u %d:%u",
            static_cast<unsigned>(i),
            symbols[i].level0,
            static_cast<unsigned>(symbols[i].duration0),
            symbols[i].level1,
            static_cast<unsigned>(symbols[i].duration1));
    }
}

void host(void*) noexcept
{
    Source::start();
    Sink::start();

    while (true) {
        ESP_ERROR_CHECK(Sink::arm());
        ESP_ERROR_CHECK(Source::send(frame));
        ESP_ERROR_CHECK(Source::wait(100));

        for (int tries = 0; tries < 50 && !Sink::ready(); ++tries) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        if (Sink::ready()) {
            dump(Sink::view());
        } else {
            ESP_LOGW(tag, "capture timeout");
        }

        vTaskDelay(sample_ticks);
    }
}

inline void boot()
{
    const auto handle = arc::spawn(
        &host,
        "trace",
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
