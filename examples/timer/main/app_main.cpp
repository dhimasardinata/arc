#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace app {

inline constexpr char tag[] = "arc-timer";
inline constexpr std::uint32_t stack = CONFIG_ARC_TIMER_STACK;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(CONFIG_ARC_TIMER_LOG_MS);
inline constexpr std::uint64_t period = static_cast<std::uint64_t>(CONFIG_ARC_TIMER_PERIOD_US);

using Led = arc::Drive<CONFIG_ARC_TIMER_LED, 0, arc::Core::core0>;
using Tick = arc::Timer<1'000'000>;

constinit static arc::TaskMem<stack> host_mem{};

struct Beat {
    IRAM_ATTR static bool alarm(const gptimer_alarm_event_data_t&) noexcept
    {
        Led::toggle();
        return false;
    }
};

void host(void*) noexcept
{
    while (true) {
        vTaskDelay(log_ticks);

        ESP_LOGI(
            tag,
            "tick=%llu us period=%llu us led=%d",
            static_cast<unsigned long long>(Tick::read()),
            static_cast<unsigned long long>(period),
            CONFIG_ARC_TIMER_LED);
    }
}

inline void boot()
{
    Led::out();
    Led::off();

    Tick::start<Beat>();
    Tick::alarm(period, 0, true);

    const auto handle = arc::spawn(
        &host,
        "timer",
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
