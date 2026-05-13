#include <cstdint>
#include <span>

#include "arc.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-copy";
inline constexpr std::uint32_t stack = 4096;
inline constexpr std::size_t bytes = 4096;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(500);

using Dma = arc::Copy<8, 64, 0, arc::CopyBackend::ahb>;

constinit static arc::TaskMem<stack> host_mem{};

[[nodiscard]] std::uint32_t sum(const std::span<const std::uint8_t> data) noexcept
{
    std::uint32_t value = 0U;
    for (const auto byte : data) {
        value += byte;
    }
    return value;
}

void host(void*) noexcept
{
    auto src = arc::dmabuf<std::uint8_t>(bytes);
    auto dst = arc::dmabuf<std::uint8_t>(bytes);
    configASSERT(static_cast<bool>(src));
    configASSERT(static_cast<bool>(dst));

    std::uint8_t seed = 0U;

    while (true) {
        for (std::size_t i = 0; i < src.size(); ++i) {
            src[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
            dst[i] = 0U;
        }

        const auto before = Dma::done();
        Dma::Ticket move{};
        ESP_ERROR_CHECK(Dma::send_coherent(move, dst.view(), src.view()));

        const auto src_sum = sum({src.data(), src.size()});
        ESP_ERROR_CHECK(Dma::finish_coherent(move));
        const auto completions = Dma::done() - before;
        const auto dst_sum = sum({dst.data(), dst.size()});

        ESP_LOGI(
            tag,
            "bytes=%u sent=%u done=%u total=%u completions=%u checksum=%s",
            static_cast<unsigned>(bytes),
            static_cast<unsigned>(Dma::sent()),
            static_cast<unsigned>(Dma::done()),
            static_cast<unsigned>(Dma::bytes()),
            static_cast<unsigned>(completions),
            src_sum == dst_sum ? "ok" : "bad");

        seed = static_cast<std::uint8_t>(seed + 1U);
        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    Dma::boot();

    const auto handle = arc::spawn(
        &host,
        "copy",
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
