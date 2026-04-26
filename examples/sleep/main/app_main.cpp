#include <cstdint>
#include <cstdio>

#include "arc.hpp"

#include "esp_check.h"

namespace app {

inline constexpr std::uint64_t nap_us = 2'000'000;

ARC_RTC static std::uint32_t boots = 0;

inline void boot()
{
    ++boots;

    const auto cause = arc::Sleep::cause();
    std::printf(
        "arc-sleep boot=%lu cause=%lu timer=%s\n",
        static_cast<unsigned long>(boots),
        static_cast<unsigned long>(cause),
        arc::Sleep::woke(ESP_SLEEP_WAKEUP_TIMER) ? "yes" : "no");

    ESP_ERROR_CHECK(arc::Sleep::after<nap_us>());
    std::printf("arc-sleep deep=%llu us\n", static_cast<unsigned long long>(nap_us));

    arc::Sleep::deep();
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
