#include <cstdio>
#include <cstdint>

#include "arc.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t period = pdMS_TO_TICKS(1000);

using Die = arc::Temp<-10, 80>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    Die::start();

    while (true) {
        const auto milli = Die::milli();
        const auto abs_milli = milli < 0 ? -milli : milli;
        std::printf(
            "arc-temp die=%s%d.%03d C\n",
            milli < 0 ? "-" : "",
            abs_milli / 1000,
            abs_milli % 1000);
        vTaskDelay(period);
    }
}

inline void boot()
{
    const auto handle = arc::spawn(
        &host,
        "temp",
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
