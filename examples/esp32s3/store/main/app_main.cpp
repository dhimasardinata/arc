#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "esp_check.h"

namespace app {

inline constexpr char tag[] = "arc-store";
inline constexpr char ns[] = "cfg";
inline constexpr char key[] = "control";
inline constexpr char name_key[] = "name";

struct Control {
    std::uint16_t pace;
    std::uint8_t mark;
    std::uint8_t flags;
};

static_assert(sizeof(Control) == 4U);

inline constexpr Control boot_cfg{
    .pace = 2000U,
    .mark = 0x31U,
    .flags = 0x01U,
};

inline void boot()
{
    ESP_ERROR_CHECK(arc::Store::boot());

    esp_err_t load = ESP_OK;
    auto cfg = arc::Store::load_or(ns, key, boot_cfg, &load);

    ESP_LOGI(
        tag,
        "load err=0x%x pace=%u mark=0x%02x flags=0x%02x",
        static_cast<unsigned>(load),
        static_cast<unsigned>(cfg.pace),
        static_cast<unsigned>(cfg.mark),
        static_cast<unsigned>(cfg.flags));

    cfg.mark ^= 0x55U;
    cfg.flags ^= 0x01U;

    ESP_ERROR_CHECK(arc::Store::save(ns, key, cfg));

    ESP_LOGI(
        tag,
        "save pace=%u mark=0x%02x flags=0x%02x",
        static_cast<unsigned>(cfg.pace),
        static_cast<unsigned>(cfg.mark),
        static_cast<unsigned>(cfg.flags));

    ESP_ERROR_CHECK(arc::Store::save_text<16>(ns, name_key, "arc-n16r8"));

    esp_err_t name_load = ESP_OK;
    auto name = arc::Store::load_text<16>(ns, name_key, "arc-default", &name_load);
    if (!name) {
        ESP_ERROR_CHECK(name.error());
    }
    ESP_LOGI(tag, "name=%s chars=%u", name->c_str(), static_cast<unsigned>(name->size()));

    arc::Space::flash(tag, "flash view");
    arc::Space::heap(tag, "heap view");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
