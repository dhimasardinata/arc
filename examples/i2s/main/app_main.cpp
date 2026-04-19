#include <array>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-i2s";
inline constexpr std::uint32_t stack = 6144;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(500);

using Link = arc::I2s<9, 10, 11, 12, 16'000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO, arc::I2sStd::msb>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    std::array<std::uint8_t, 256> tx{};
    std::array<std::uint8_t, 256> rx{};

    for (std::size_t i = 0; i < tx.size(); ++i) {
        tx[i] = static_cast<std::uint8_t>((i * 17U) & 0xFFU);
    }

    std::size_t loaded = 0U;
    const auto preload = Link::preload(tx.data(), tx.size(), &loaded);
    configASSERT(preload == ESP_OK || preload == ESP_ERR_INVALID_SIZE);

    Link::start();

    while (true) {
        std::size_t wrote = 0U;
        std::size_t got = 0U;

        const auto wret = Link::write(tx.data(), tx.size(), &wrote, 1000U);
        const auto rret = Link::read(rx.data(), rx.size(), &got, 1000U);

        if (wret != ESP_OK || rret != ESP_OK) {
            ESP_LOGW(
                tag,
                "io failed w=0x%x r=0x%x sent=%u recv=%u sovf=%u rovf=%u",
                static_cast<unsigned>(wret),
                static_cast<unsigned>(rret),
                static_cast<unsigned>(Link::sent()),
                static_cast<unsigned>(Link::recv()),
                static_cast<unsigned>(Link::send_ovf()),
                static_cast<unsigned>(Link::recv_ovf()));
            vTaskDelay(log_ticks);
            continue;
        }

        ESP_LOGI(
            tag,
            "rate=%u wrote=%u read=%u sent=%u recv=%u first=%02x %02x %02x %02x",
            static_cast<unsigned>(Link::rate()),
            static_cast<unsigned>(wrote),
            static_cast<unsigned>(got),
            static_cast<unsigned>(Link::sent()),
            static_cast<unsigned>(Link::recv()),
            rx[0],
            rx[1],
            rx[2],
            rx[3]);

        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    Link::boot();

    const auto handle = arc::spawn(
        &host,
        "i2s",
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
