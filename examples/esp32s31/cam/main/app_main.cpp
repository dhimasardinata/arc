#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/dma_chain.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("esp_cam_ctlr.h") && __has_include("esp_cam_ctlr_dvp.h") && __has_include("esp_lcd_panel_ops.h") && __has_include("esp_lcd_panel_rgb.h")
#include "arc/dvp.hpp"
#include "arc/rgb.hpp"
#define ARC_S31_KORVO_CAM_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_CAM_DRIVER_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

#if ARC_S31_KORVO_CAM_DRIVER_CONTRACT
using CamPath = arc::Dvp<
    arc::DvpLines<
        Board::Cam::d0,
        Board::Cam::d1,
        Board::Cam::d2,
        Board::Cam::d3,
        Board::Cam::d4,
        Board::Cam::d5,
        Board::Cam::d6,
        Board::Cam::d7>,
    Board::Cam::vsync,
    Board::Cam::pclk,
    Board::Cam::hsync,
    Board::Cam::xclk,
    320,
    240>;

using LcdPath = arc::Rgb<
    arc::RgbLines<
        Board::Lcd::db0,
        Board::Lcd::db1,
        Board::Lcd::db2,
        Board::Lcd::db3,
        Board::Lcd::db4,
        Board::Lcd::db5,
        Board::Lcd::db6,
        Board::Lcd::db7,
        Board::Lcd::db8,
        Board::Lcd::db9,
        Board::Lcd::db10,
        Board::Lcd::db11,
        Board::Lcd::db12,
        Board::Lcd::db13,
        Board::Lcd::db14,
        Board::Lcd::db15>,
    Board::Lcd::hsync,
    Board::Lcd::vsync,
    Board::Lcd::hen,
    Board::Lcd::pclk,
    -1,
    Board::Display::hres,
    Board::Display::vres>;

static_assert(CamPath::width() == 8U);
static_assert(LcdPath::width() == 16U);
static_assert(LcdPath::h() == Board::Display::hres);
static_assert(LcdPath::v() == Board::Display::vres);
static_assert(sizeof(Board::Resource::CamBus) > 0U);
static_assert(sizeof(Board::Resource::LcdBus) > 0U);
static_assert(sizeof(CamPath::Resource) > 0U);
static_assert(sizeof(LcdPath::Resource) > 0U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "cam requires ARC_TARGET=esp32s31");
    static_assert(arc::soc::Target::camera && arc::soc::Target::display, "ESP32-S31 scaffold expects camera/display capability");
    static_assert(arc::Soc::dvp && arc::Soc::lcd_rgb, "ESP32-S31 camera scaffold expects SDK DVP/RGB LCD capabilities");
    static_assert(arc::Topology<Board>);
    static_assert(arc::board::Korvo1CamGraph::valid());
    static_assert(arc::board::Korvo1LcdGraph::valid());
    static_assert(Board::Cam::d0 == 46 && Board::Cam::d7 == 53, "Korvo camera data bus must stay contiguous");
    static_assert(Board::CamModule::external && Board::CamModule::connector, "Korvo camera accessory topology changed");
    static_assert(Board::CamModule::ldo_in_mv == 3300U, "Korvo camera LDO input changed");
    static_assert(Board::CamModule::avdd_mv == 2800U && Board::CamModule::dvdd_mv == 1500U, "Korvo camera power rails changed");
    static_assert(Board::CamModule::avdd_ldo && Board::CamModule::dvdd_ldo, "Korvo camera LDO topology changed");
    static_assert(Board::CamModule::video_stream && Board::CamModule::jpeg_stream, "Korvo camera streaming contract changed");
    static_assert(Board::Lcd::db0 == 8 && Board::Lcd::db15 == 36, "Korvo LCD data bus changed");
    static_assert(Board::Display::external && Board::Display::connector, "Korvo display accessory topology changed");
    static_assert(std::string_view{Board::Display::panel_driver} == "ST7262E43", "Korvo LCD panel driver changed");
    static_assert(std::string_view{Board::Display::touch_driver} == "GT1151", "Korvo LCD touch driver changed");
    static_assert(Board::Display::hres == 800U && Board::Display::vres == 480U, "Korvo LCD resolution changed");
    static_assert(Board::Display::rgb && Board::Display::rgb565 && Board::Display::touch, "Korvo LCD interface changed");

    std::array<std::uint8_t, 64> line{};
    arc::DmaChain<2> chain{};
    chain.bind(0, std::span<std::uint8_t>{line});
    chain.link_circular();

    std::printf(
        "arc-s31-cam target=%s board=%s arch=%s cam_module=%s cam=%d..%d cam_ldo_in_mv=%u cam_avdd_mv=%u cam_dvdd_mv=%u cam_avdd_ldo=%d cam_dvdd_ldo=%d video_stream=%d jpeg_stream=%d display=%s panel=%s touch=%s res=%ux%u lcd=%d..%d cam_edges=%zu lcd_edges=%zu driver_contract=%d dma_head=%p ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::CamModule::model,
        Board::Cam::d0,
        Board::Cam::d7,
        static_cast<unsigned>(Board::CamModule::ldo_in_mv),
        static_cast<unsigned>(Board::CamModule::avdd_mv),
        static_cast<unsigned>(Board::CamModule::dvdd_mv),
        Board::CamModule::avdd_ldo ? 1 : 0,
        Board::CamModule::dvdd_ldo ? 1 : 0,
        Board::CamModule::video_stream ? 1 : 0,
        Board::CamModule::jpeg_stream ? 1 : 0,
        Board::Display::expansion,
        Board::Display::panel_driver,
        Board::Display::touch_driver,
        static_cast<unsigned>(Board::Display::hres),
        static_cast<unsigned>(Board::Display::vres),
        Board::Lcd::db0,
        Board::Lcd::db15,
        arc::board::Korvo1CamGraph::edge_count,
        arc::board::Korvo1LcdGraph::edge_count,
        ARC_S31_KORVO_CAM_DRIVER_CONTRACT,
        static_cast<void*>(chain.head()),
        arc::soc::has<arc::soc::Cap::cam> ? 1 : 0);
}
