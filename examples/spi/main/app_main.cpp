#include <array>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-spi";
inline constexpr std::uint32_t stack = 4096;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(500);

using Bus = arc::SpiBus<SPI2_HOST, 12, 11, 13>;
using Dev = arc::Spi<Bus, -1, SPI_MASTER_FREQ_20M, 0>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    std::array<std::uint8_t, 16> tx{};
    auto rx = arc::dmabuf<std::uint8_t>(tx.size());
    configASSERT(static_cast<bool>(rx));

    std::uint8_t seed = 0U;

    while (true) {
        for (std::size_t i = 0; i < tx.size(); ++i) {
            tx[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
        }

        const auto ret = Dev::poll(tx.data(), rx.data(), tx.size());
        if (ret != ESP_OK) {
            ESP_LOGW(tag, "poll failed ret=0x%x", static_cast<unsigned>(ret));
            vTaskDelay(log_ticks);
            continue;
        }

        bool match = true;
        for (std::size_t i = 0; i < tx.size(); ++i) {
            if (tx[i] != rx[i]) {
                match = false;
                break;
            }
        }

        ESP_LOGI(
            tag,
            "host=%d sclk=%d mosi=%d miso=%d hz=%u echo=%s first=%02x %02x %02x %02x",
            static_cast<int>(Bus::host()),
            Bus::sclk(),
            Bus::mosi(),
            Bus::miso(),
            Dev::hz(),
            match ? "ok" : "mismatch",
            rx[0],
            rx[1],
            rx[2],
            rx[3]);

        seed = static_cast<std::uint8_t>(seed + 1U);
        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    Dev::boot();

    const auto handle = arc::spawn(
        &host,
        "spi",
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
