#include <array>
#include <cstddef>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"

namespace app {

inline constexpr char tag[] = "arc-space";
inline constexpr std::size_t hot_bytes = 32U * 1024U;
inline constexpr std::size_t cold_bytes = 512U * 1024U;
inline constexpr std::size_t dma_bytes = 8U * 1024U;
inline constexpr std::size_t simd_words = 2048U;

ARC_DMA std::array<std::uint8_t, 1024> dma_static{};
ARC_RTC_NOINIT std::uint32_t boots;

struct Hot {
    std::array<std::uint8_t, hot_bytes> bytes{};
};

struct Cold {
    std::array<std::uint8_t, cold_bytes> bytes{};
};

inline arc::CapsPtr<Hot> hot{};
inline arc::CapsPtr<Cold> cold{};
inline arc::CapsBuf<std::uint8_t> dma{};
inline arc::CapsBuf<std::uint16_t> simd{};

inline void boot()
{
    ++boots;
    arc::Space::all(tag, "baseline");

    ESP_LOGI(
        tag,
        "placed static dma=%uB rtc boots=%u",
        static_cast<unsigned>(dma_static.size()),
        static_cast<unsigned>(boots));

    hot = arc::inram<Hot>();
    cold = arc::psram<Cold>();
    dma = arc::dmabuf<std::uint8_t>(dma_bytes);
    simd = arc::simdbuf<std::uint16_t>(simd_words);

    ESP_LOGI(
        tag,
        "alloc hot=%uB internal=%s cold=%uB psram=%s dma=%uB %s simd=%uB %s",
        static_cast<unsigned>(sizeof(Hot)),
        hot != nullptr ? "ok" : "fail",
        static_cast<unsigned>(sizeof(Cold)),
        cold != nullptr ? "ok" : "fail",
        static_cast<unsigned>(dma.bytes()),
        dma ? "ok" : "fail",
        static_cast<unsigned>(simd.bytes()),
        simd ? "ok" : "fail");

    arc::Space::heap(tag, "after explicit arc::inram + arc::psram + arc::dmabuf + arc::simdbuf");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
