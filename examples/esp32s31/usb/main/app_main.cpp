#include <cstdint>
#include <cstdio>
#include <type_traits>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"
#include "arc/usb_device.hpp"
#include "arc/usb_host.hpp"

#if __has_include("esp_private/usb_phy.h")
#include "arc/otg.hpp"
#define ARC_S31_KORVO_USB_PHY_CONTRACT 1
#else
#define ARC_S31_KORVO_USB_PHY_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

constexpr auto device_descriptor = arc::usb::DeviceDescriptor{
    .klass = arc::usb::Class::vendor,
    .vendor = 0x303aU,
    .product = 0x5031U,
}
                                       .bytes();
constexpr auto cdc_descriptor = arc::usb::Cdc<0x83U, 0x01U, 0x82U>::descriptors();
constexpr auto host_config = arc::usb::HostConfig{.port = 1U, .max_packet = 64U};

static_assert(device_descriptor[0] == 18U);
static_assert(cdc_descriptor.size() == 48U);
static_assert(host_config.port == 1U && host_config.max_packet == 64U);
static_assert(Board::Usb::dp_module_pin == 40);
static_assert(Board::Usb::dm_module_pin == 41);
static_assert(Board::Usb::dp_gpio == -1 && Board::Usb::dm_gpio == -1);
static_assert(!Board::Usb::module_pins_are_gpio);
static_assert(!Board::pins::has<Board::Usb::dp_gpio>());
static_assert(!Board::pins::has<Board::Usb::dm_gpio>());
static_assert(Board::UsbHost::type_a);
static_assert(Board::UsbHost::high_speed);
static_assert(Board::UsbHost::downstream_power);
static_assert(Board::UsbHost::current_limited);
static_assert(Board::UsbHost::downstream_ma == 500U);
static_assert(host_config.port == Board::UsbHost::port);
static_assert(Board::Power::input_ma == 3'000U);
static_assert(Board::Power::buck_3v3 && Board::Power::power_led_5v);

#if ARC_S31_KORVO_USB_PHY_CONTRACT
using UsbPhy = arc::Otg;
using UsbMode = UsbPhy::Mode;
static_assert(Board::Onboard::usb_otg);
static_assert(std::is_same_v<Board::Resource::UsbOtg, UsbPhy::Resource>);
static_assert(sizeof(Board::Resource::UsbOtg) > 0U);
static_assert(sizeof(UsbPhy::Resource) > 0U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "usb requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);
    static_assert(arc::soc::Target::usb_otg, "ESP32-S31 USB scaffold expects USB OTG capability");
    static_assert(arc::Soc::usb_otg, "ESP32-S31 USB scaffold expects SDK USB OTG capability");
    static_assert(Board::Onboard::usb_otg, "Korvo USB OTG fact changed");

    std::printf(
        "arc-s31-usb target=%s board=%s arch=%s otg=%d phy_contract=%d usb_dp_module_pin=%d usb_dm_module_pin=%d host_type_a=%d host_high_speed=%d host_switch=%s host_current_limit=%d host_downstream_ma=%u input_ma=%u buck_3v3=%d device_desc=%u cdc_desc=%zu host_port=%u ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        arc::soc::Target::usb_otg ? 1 : 0,
        ARC_S31_KORVO_USB_PHY_CONTRACT,
        Board::Usb::dp_module_pin,
        Board::Usb::dm_module_pin,
        Board::UsbHost::type_a ? 1 : 0,
        Board::UsbHost::high_speed ? 1 : 0,
        Board::UsbHost::switch_model,
        Board::UsbHost::current_limited ? 1 : 0,
        static_cast<unsigned>(Board::UsbHost::downstream_ma),
        static_cast<unsigned>(Board::Power::input_ma),
        Board::Power::buck_3v3 ? 1 : 0,
        static_cast<unsigned>(device_descriptor[0]),
        cdc_descriptor.size(),
        static_cast<unsigned>(host_config.port),
        Board::Onboard::usb_otg ? 1 : 0);
}
