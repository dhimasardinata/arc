#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-bridge";
inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(1000);

inline constexpr int high_pin = 4;
inline constexpr int low_pin = 5;

using Half = arc::Bridge<high_pin, low_pin, 20'000, 400, 50, 100>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    while (true) {
        vTaskDelay(log_ticks);

        ESP_LOGI(
            tag,
            "hi=%d lo=%d hz=%u period=%u ticks duty=%u/1000 red=%u fed=%u",
            high_pin,
            low_pin,
            20'000U,
            static_cast<unsigned>(Half::period()),
            static_cast<unsigned>(Half::permille()),
            50U,
            100U);
    }
}

inline void boot()
{
    Half::start();

    const auto handle = arc::spawn(
        &host,
        "bridge",
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
