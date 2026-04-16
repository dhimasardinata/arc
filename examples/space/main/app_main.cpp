#include <array>
#include <cstddef>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"

namespace app {

inline constexpr char tag[] = "arc-space";
inline constexpr std::size_t hot_bytes = 32U * 1024U;
inline constexpr std::size_t cold_bytes = 512U * 1024U;

struct Hot {
    std::array<std::uint8_t, hot_bytes> bytes{};
};

struct Cold {
    std::array<std::uint8_t, cold_bytes> bytes{};
};

inline arc::CapsPtr<Hot> hot{};
inline arc::CapsPtr<Cold> cold{};

inline void boot()
{
    arc::Space::all(tag, "baseline");

    hot = arc::inram<Hot>();
    cold = arc::psram<Cold>();

    ESP_LOGI(
        tag,
        "alloc hot=%uB internal=%s cold=%uB psram=%s",
        static_cast<unsigned>(sizeof(Hot)),
        hot != nullptr ? "ok" : "fail",
        static_cast<unsigned>(sizeof(Cold)),
        cold != nullptr ? "ok" : "fail");

    arc::Space::heap(tag, "after explicit arc::inram + arc::psram");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
