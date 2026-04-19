#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"

namespace app {

inline constexpr char tag[] = "arc-etm";
inline constexpr int out_pin = 4;
inline constexpr std::uint64_t half_us = 100;

using Clock = arc::Timer<1'000'000>;
using Blink = arc::Route<arc::Alarm<Clock>, arc::Toggle<out_pin>>;
using Rearm = arc::Route<arc::Alarm<Clock>, arc::Arm<Clock>>;
using Reset = arc::Route<arc::Alarm<Clock>, arc::Reload<Clock>>;

inline void boot()
{
    Clock::alarm(half_us, 0, false);

    Blink::on();
    Rearm::on();
    Reset::on();

    Clock::start();

    ESP_LOGI(
        tag,
        "pin=%d half=%llu us full=%llu us via ETM",
        out_pin,
        static_cast<unsigned long long>(half_us),
        static_cast<unsigned long long>(half_us * 2U));
    arc::Etm::dump();
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
