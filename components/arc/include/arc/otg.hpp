#pragma once

#include <utility>

#include "esp_err.h"
#include "esp_private/usb_phy.h"

#include "arc/result.hpp"
#include "arc/soc.hpp"

namespace arc {

struct Otg {
    static_assert(Soc::usb_otg, "USB OTG is not supported on this target");

    using Config = usb_phy_config_t;
    using Handle = usb_phy_handle_t;
    using Io = usb_phy_otg_io_conf_t;
    using Ext = usb_phy_ext_io_conf_t;
    using Mode = usb_otg_mode_t;
    using Speed = usb_phy_speed_t;
    using Target = usb_phy_target_t;
    using Status = usb_phy_status_t;

    constexpr Otg() noexcept = default;

    explicit constexpr Otg(const Handle handle) noexcept
        : handle_(handle)
    {
    }

    Otg(const Otg&) = delete;
    Otg& operator=(const Otg&) = delete;

    Otg(Otg&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
    {
    }

    Otg& operator=(Otg&& other) noexcept
    {
        if (this != &other) {
            static_cast<void>(off());
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~Otg()
    {
        static_cast<void>(off());
    }

    [[nodiscard]] static Result<Otg> make(const Config& config) noexcept
    {
        Handle handle{};
        const auto err = usb_new_phy(&config, &handle);
        if (err != ESP_OK) {
            return fail(err);
        }
        return Otg{handle};
    }

    [[nodiscard]] static Result<Otg> host(
        const Target target = USB_PHY_TARGET_INT,
        const Speed speed = USB_PHY_SPEED_UNDEFINED,
        const Io* const io = nullptr,
        const Ext* const ext = nullptr) noexcept
    {
        const Config config{
            .controller = USB_PHY_CTRL_OTG,
            .target = target,
            .otg_mode = USB_OTG_MODE_HOST,
            .otg_speed = speed,
            .ext_io_conf = ext,
            .otg_io_conf = io,
        };
        return make(config);
    }

    [[nodiscard]] static Result<Otg> device(
        const Target target = USB_PHY_TARGET_INT,
        const Speed speed = USB_PHY_SPEED_FULL,
        const Io* const io = nullptr,
        const Ext* const ext = nullptr) noexcept
    {
        const Config config{
            .controller = USB_PHY_CTRL_OTG,
            .target = target,
            .otg_mode = USB_OTG_MODE_DEVICE,
            .otg_speed = speed,
            .ext_io_conf = ext,
            .otg_io_conf = io,
        };
        return make(config);
    }

    [[nodiscard]] esp_err_t mode(const Mode value) const noexcept
    {
        return handle_ == nullptr ? ESP_ERR_INVALID_STATE : usb_phy_otg_set_mode(handle_, value);
    }

    [[nodiscard]] esp_err_t off() noexcept
    {
        if (handle_ == nullptr) {
            return ESP_OK;
        }

        const auto raw = std::exchange(handle_, nullptr);
        return usb_del_phy(raw);
    }

    [[nodiscard]] Handle native() const noexcept
    {
        return handle_;
    }

    explicit constexpr operator bool() const noexcept
    {
        return handle_ != nullptr;
    }

    [[nodiscard]] static Result<Status> status(const Target target = USB_PHY_TARGET_INT) noexcept
    {
        Status out{};
        const auto err = usb_phy_get_phy_status(target, &out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

private:
    Handle handle_{};
};

}  // namespace arc
