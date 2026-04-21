#include <array>
#include <cstdint>
#include <span>

#include "arc.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-can";
inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t period = pdMS_TO_TICKS(1000);

using Bus = arc::Can<
    4,
    4,
    500'000,
    4,
    8,
    true,
    true>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    Bus::start();

    std::uint32_t seq = 0U;
    while (true) {
        std::array<std::uint8_t, 8> payload{
            static_cast<std::uint8_t>(seq),
            static_cast<std::uint8_t>(seq >> 8U),
            static_cast<std::uint8_t>(seq >> 16U),
            static_cast<std::uint8_t>(seq >> 24U),
            0xA5U,
            0x5AU,
            0xC3U,
            0x3CU,
        };

        auto tx = Bus::frame(0x123U, std::span<const std::uint8_t>{payload.data(), payload.size()});
        ESP_ERROR_CHECK(Bus::send_wait(tx, 1000));

        Bus::Frame rx{};
        while (Bus::recv(rx)) {
            ESP_LOGI(
                tag,
                "id=0x%03x len=%u sent=%u done=%u rx=%u drop=%u err=%u",
                static_cast<unsigned>(rx.header.id),
                static_cast<unsigned>(rx.bytes),
                static_cast<unsigned>(Bus::sent()),
                static_cast<unsigned>(Bus::done()),
                static_cast<unsigned>(Bus::rx()),
                static_cast<unsigned>(Bus::drop()),
                static_cast<unsigned>(Bus::err()));
        }

        ++seq;
        vTaskDelay(period);
    }
}

inline void boot()
{
    const auto handle = arc::spawn(
        &host,
        "can",
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
