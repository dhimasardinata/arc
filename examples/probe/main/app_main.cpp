#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-probe";
inline constexpr std::uint32_t host_stack = 4096;
inline constexpr std::uint32_t realtime_stack = 4096;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(1000);
inline constexpr int led = 4;

using Led = arc::Drive<led, 0, arc::Core::core1>;

struct Snapshot {
    std::uint32_t samples;
    std::uint32_t min;
    std::uint32_t avg;
    std::uint32_t max;
};

constinit static arc::SeqReg<Snapshot> report{};
constinit static arc::CycleStats cycles{};
constinit static arc::TaskMem<host_stack> host_mem{};

struct Hot {
    static void setup()
    {
        Led::out();
        Led::off();
    }

    IRAM_ATTR static void loop() noexcept
    {
        const auto mark = arc::Probe::now();
        Led::on();
        Led::off();
        cycles.add(mark.lap());

        if ((cycles.samples & 0x3FFFU) == 0U) {
            report.write(Snapshot{
                .samples = cycles.samples,
                .min = cycles.min,
                .avg = cycles.avg(),
                .max = cycles.max,
            });
        }
    }
};

using Core1 = arc::App<Hot, realtime_stack, arc::Core::core1>;

void host(void*) noexcept
{
    while (true) {
        const auto snap = report.read();
        ESP_LOGI(
            tag,
            "samples=%u min=%u avg=%u max=%u cycles led=%d",
            static_cast<unsigned>(snap.samples),
            static_cast<unsigned>(snap.min),
            static_cast<unsigned>(snap.avg),
            static_cast<unsigned>(snap.max),
            led);
        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    const auto handle = arc::spawn(
        &host,
        "probe",
        nullptr,
        1,
        arc::Core::core0,
        host_mem);
    configASSERT(handle != nullptr);

    Core1::boot(tag);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
