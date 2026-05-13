#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-sigma";
inline constexpr std::uint32_t pin = 4;
inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t step_ticks = pdMS_TO_TICKS(250);

using Analog = arc::Sigma<pin, 1'000'000, 0>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    int value = -90;
    int step = 15;

    while (true) {
        Analog::write(value);

        ESP_LOGI(
            tag,
            "pin=%u sample=%uHz density=%d level=%d",
            static_cast<unsigned>(pin),
            static_cast<unsigned>(Analog::rate()),
            value,
            static_cast<int>(Analog::level()));

        value += step;
        if (value >= 90) {
            value = 90;
            step = -step;
        } else if (value <= -90) {
            value = -90;
            step = -step;
        }

        vTaskDelay(step_ticks);
    }
}

inline void boot()
{
    Analog::start();

    const auto handle = arc::spawn(
        &host,
        "sigma",
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
