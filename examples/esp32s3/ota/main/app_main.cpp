#include "arc.hpp"

#include "esp_log.h"

namespace app {

inline constexpr char tag[] = "arc-ota";

[[nodiscard]] constexpr const char* state_name(const esp_ota_img_states_t state) noexcept
{
    switch (state) {
        case ESP_OTA_IMG_NEW:
            return "new";
        case ESP_OTA_IMG_PENDING_VERIFY:
            return "pending_verify";
        case ESP_OTA_IMG_VALID:
            return "valid";
        case ESP_OTA_IMG_INVALID:
            return "invalid";
        case ESP_OTA_IMG_ABORTED:
            return "aborted";
        case ESP_OTA_IMG_UNDEFINED:
        default:
            return "undefined";
    }
}

inline void boot()
{
    const auto* const running = arc::Ota::running();
    const auto* const boot_part = arc::Ota::boot();
    const auto* const next = arc::Ota::next();

    esp_ota_img_states_t running_state{};
    const auto state_err = arc::Ota::state(running, running_state);
    esp_err_t pending_err = ESP_OK;
    const auto pending = arc::Ota::pending_verify(&pending_err);

    ESP_LOGI(
        tag,
        "running=%s@0x%08x boot=%s@0x%08x next=%s@0x%08x state=%s state_err=0x%x pending=%s pending_err=0x%x",
        running != nullptr ? running->label : "-",
        running != nullptr ? running->address : 0U,
        boot_part != nullptr ? boot_part->label : "-",
        boot_part != nullptr ? boot_part->address : 0U,
        next != nullptr ? next->label : "-",
        next != nullptr ? next->address : 0U,
        state_name(running_state),
        static_cast<unsigned>(state_err),
        pending ? "yes" : "no",
        static_cast<unsigned>(pending_err));

    arc::Space::flash(tag, "flash + ota view");
    arc::Space::parts(tag, "partitions");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
