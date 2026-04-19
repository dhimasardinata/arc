#include <array>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-scope";
inline constexpr std::uint32_t stack = 6144;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(250);

using In = arc::Scope<40'000, 256, 4096, true, arc::Adc<4>>;

constinit static arc::TaskMem<stack> host_mem{};

void host(void*) noexcept
{
    std::array<adc_continuous_data_t, 64> frame{};

    while (true) {
        std::uint32_t got = 0U;
        const auto ret = In::pull(frame.data(), static_cast<std::uint32_t>(frame.size()), &got, 1000);

        if (ret == ESP_ERR_TIMEOUT) {
            continue;
        }

        if (ret != ESP_OK) {
            ESP_LOGW(tag, "pull failed ret=0x%x overruns=%u", static_cast<unsigned>(ret), static_cast<unsigned>(In::overruns()));
            vTaskDelay(log_ticks);
            continue;
        }

        std::uint32_t min = 0xFFFFU;
        std::uint32_t max = 0U;
        std::uint32_t sum = 0U;
        std::uint32_t valid = 0U;

        for (std::uint32_t i = 0; i < got; ++i) {
            const auto& sample = frame[static_cast<std::size_t>(i)];
            if (!sample.valid) {
                continue;
            }

            const auto raw = sample.raw_data;
            min = raw < min ? raw : min;
            max = raw > max ? raw : max;
            sum += raw;
            valid += 1U;
        }

        if (valid == 0U) {
            ESP_LOGW(tag, "no valid samples got=%u frames=%u", static_cast<unsigned>(got), static_cast<unsigned>(In::frames()));
            vTaskDelay(log_ticks);
            continue;
        }

        ESP_LOGI(
            tag,
            "io=%d hz=%u samples=%u avg=%u min=%u max=%u frames=%u ovf=%u",
            arc::Adc<4>::io(),
            In::hz(),
            static_cast<unsigned>(valid),
            static_cast<unsigned>(sum / valid),
            static_cast<unsigned>(min),
            static_cast<unsigned>(max),
            static_cast<unsigned>(In::frames()),
            static_cast<unsigned>(In::overruns()));

        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    In::start();

    const auto handle = arc::spawn(
        &host,
        "scope",
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
