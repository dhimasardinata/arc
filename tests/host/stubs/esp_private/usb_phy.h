#pragma once

#include "esp_err.h"

using usb_phy_handle_t = void*;

enum usb_phy_controller_t {
    USB_PHY_CTRL_OTG = 0,
};

enum usb_phy_target_t {
    USB_PHY_TARGET_INT = 0,
    USB_PHY_TARGET_EXT = 1,
};

enum usb_otg_mode_t {
    USB_OTG_MODE_HOST = 0,
    USB_OTG_MODE_DEVICE = 1,
};

enum usb_phy_speed_t {
    USB_PHY_SPEED_UNDEFINED = 0,
    USB_PHY_SPEED_FULL = 1,
};

enum usb_phy_status_t {
    USB_PHY_STATUS_READY = 0,
};

struct usb_phy_otg_io_conf_t {};

struct usb_phy_ext_io_conf_t {};

struct usb_phy_config_t {
    usb_phy_controller_t controller{USB_PHY_CTRL_OTG};
    usb_phy_target_t target{USB_PHY_TARGET_INT};
    usb_otg_mode_t otg_mode{USB_OTG_MODE_DEVICE};
    usb_phy_speed_t otg_speed{USB_PHY_SPEED_FULL};
    const usb_phy_ext_io_conf_t* ext_io_conf{};
    const usb_phy_otg_io_conf_t* otg_io_conf{};
};

inline esp_err_t usb_new_phy(
    const usb_phy_config_t* const config,
    usb_phy_handle_t* const out)
{
    if (config == nullptr || out == nullptr || config->controller != USB_PHY_CTRL_OTG) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = reinterpret_cast<usb_phy_handle_t>(0x7100);
    return ESP_OK;
}

inline esp_err_t usb_del_phy(const usb_phy_handle_t handle)
{
    return handle != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t usb_phy_otg_set_mode(
    const usb_phy_handle_t handle,
    const usb_otg_mode_t)
{
    return handle != nullptr ? ESP_OK : ESP_ERR_INVALID_ARG;
}

inline esp_err_t usb_phy_get_phy_status(
    const usb_phy_target_t,
    usb_phy_status_t* const out)
{
    if (out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    *out = USB_PHY_STATUS_READY;
    return ESP_OK;
}
