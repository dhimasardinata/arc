#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-pulse";
inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(500);

inline constexpr int out_pin = 4;
inline constexpr int in_pin = 5;

using Out = arc::Pulse<out_pin, 20'000, 250>;
using In = arc::Capture<in_pin, 1'000'000>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    while (true) {
        vTaskDelay(log_ticks);

        const auto tick = In::ticks();
        const auto period = In::period();
        const auto high = In::high();
        const auto low = In::low();
        const auto edges = In::edges();
        const auto rise = In::rising();
        const auto ready = In::ready();

        ESP_LOGI(
            tag,
            "out=%d in=%d ready=%d edge=%s count=%u tick=%u us period=%u us high=%u us low=%u us duty=%u/1000",
            out_pin,
            in_pin,
            ready ? 1 : 0,
            rise ? "rise" : "fall",
            static_cast<unsigned>(edges),
            static_cast<unsigned>(tick),
            static_cast<unsigned>(period),
            static_cast<unsigned>(high),
            static_cast<unsigned>(low),
            static_cast<unsigned>(Out::permille()));
        In::flush();
    }
}

inline void boot()
{
    Out::start();
    In::start();

    const auto handle = arc::spawn(
        &host,
        "pulse",
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
