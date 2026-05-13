#include <cstdint>

#include "arc.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-i80";
inline constexpr std::uint32_t stack = 4096;
inline constexpr std::size_t pixels = 128;
inline constexpr TickType_t period = pdMS_TO_TICKS(500);

using Bus = arc::I80Bus<
    arc::Lines<4, 5, 6, 7, 15, 16, 17, 18>,
    21,
    47,
    pixels * sizeof(std::uint16_t)>;

using Lcd = arc::I80<Bus, 48, 20'000'000, 4, 8, 8, true>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    auto frame = Lcd::buffer<std::uint16_t>(pixels);
    configASSERT(static_cast<bool>(frame));

    std::uint16_t phase = 0U;

    while (true) {
        for (std::size_t i = 0; i < frame.size(); ++i) {
            const auto v = static_cast<std::uint16_t>(phase + static_cast<std::uint16_t>(i));
            frame[i] = static_cast<std::uint16_t>(((v & 0x1fU) << 11U) | (((v >> 1U) & 0x3fU) << 5U) | (v & 0x1fU));
        }

        Lcd::Ticket paint{};
        ESP_ERROR_CHECK(Lcd::color_coherent(paint, 0x2C, frame));
        Lcd::finish(paint);
        ESP_LOGI(
            tag,
            "hz=%u sent=%u done=%u frame=%u total=%u",
            static_cast<unsigned>(Lcd::hz()),
            static_cast<unsigned>(Lcd::sent()),
            static_cast<unsigned>(Lcd::done()),
            static_cast<unsigned>(frame.bytes()),
            static_cast<unsigned>(Lcd::bytes()));

        phase = static_cast<std::uint16_t>(phase + 1U);
        vTaskDelay(period);
    }
}

inline void boot()
{
    Lcd::boot();

    const auto handle = arc::spawn(
        &host,
        "i80",
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
