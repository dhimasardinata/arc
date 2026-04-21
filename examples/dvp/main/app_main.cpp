#include <cstddef>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace app {

inline constexpr char tag[] = "arc-dvp";
inline constexpr std::size_t pixels = 160U * 120U;

using Cam = arc::Dvp<
    arc::DvpLines<4, 5, 6, 7, 15, 16, 17, 18>,
    39,
    40,
    41,
    42,
    160,
    120,
    CAM_CTLR_COLOR_RGB565,
    CAM_CTLR_COLOR_RGB565,
    20'000'000,
    64,
    false,
    false,
    false,
    false>;

inline void boot()
{
    Cam::boot();

    auto frame = Cam::buffer<std::uint16_t>(pixels);
    configASSERT(static_cast<bool>(frame));

    ESP_LOGI(
        tag,
        "dvp %ux%u width=%u frame=%uB",
        static_cast<unsigned>(Cam::h()),
        static_cast<unsigned>(Cam::v()),
        static_cast<unsigned>(Cam::width()),
        static_cast<unsigned>(frame.bytes()));
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
