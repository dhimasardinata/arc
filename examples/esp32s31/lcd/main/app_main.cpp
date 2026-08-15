#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/dma_chain.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("esp_lcd_panel_ops.h") && __has_include("esp_lcd_panel_rgb.h")
#include "arc/rgb.hpp"
#define ARC_S31_KORVO_LCD_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_LCD_DRIVER_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

[[nodiscard]] consteval std::array<std::uint16_t, 16> lcd_probe_frame() noexcept
{
    std::array<std::uint16_t, 16> out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint16_t>(0x0010U + i);
    }
    return out;
}

inline constexpr auto lcd_probe = lcd_probe_frame();

#if ARC_S31_KORVO_LCD_DRIVER_CONTRACT
using LcdPanel = arc::Rgb<
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

static_assert(LcdPanel::width() == 16U);
static_assert(LcdPanel::h() == Board::Display::hres);
static_assert(LcdPanel::v() == Board::Display::vres);
static_assert(LcdPanel::fbs() == 1U);
static_assert(sizeof(LcdPanel::Resource) > 0U);
#endif

static_assert(lcd_probe.front() == 0x0010U);
static_assert(lcd_probe.back() == 0x001fU);

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "lcd requires ARC_TARGET=esp32s31");
    static_assert(arc::soc::Target::display, "ESP32-S31 LCD scaffold expects display capability");
    static_assert(arc::Soc::lcd_rgb, "ESP32-S31 LCD scaffold expects SDK RGB LCD capability");
    static_assert(arc::Topology<Board>);
    static_assert(arc::board::Korvo1LcdGraph::valid());
    static_assert(arc::board::Korvo1StrapGraph::valid());
    static_assert(Board::Onboard::lcd, "Korvo LCD fact changed");
    static_assert(Board::Display::external && Board::Display::connector, "Korvo LCD expansion topology changed");
    static_assert(std::string_view{Board::Display::panel_driver} == "ST7262E43", "Korvo LCD panel driver changed");
    static_assert(std::string_view{Board::Display::touch_driver} == "GT1151", "Korvo LCD touch driver changed");
    static_assert(Board::Display::inch_x10 == 43U, "Korvo LCD physical size changed");
    static_assert(Board::Display::hres == 800U && Board::Display::vres == 480U, "Korvo LCD resolution changed");
    static_assert(Board::Display::rgb && Board::Display::rgb565 && Board::Display::touch, "Korvo LCD interface changed");
    static_assert(Board::Lcd::db0 == 8 && Board::Lcd::db15 == 36, "Korvo LCD data bus changed");
    static_assert(Board::Lcd::pclk == 40 && Board::Lcd::vsync == 45, "Korvo LCD sync pins changed");
    static_assert(Board::Lcd::db15 == Board::Strap::lcd_db15, "Korvo LCD DB15 strap mapping changed");
    static_assert(Board::Lcd::cs == 38 && Board::Lcd::mosi == 60 && Board::Lcd::sck == 61, "Korvo LCD control SPI pins changed");
    static_assert(Board::Lcd::cs == Board::Strap::b0, "Korvo LCD CS strap mapping changed");
    static_assert(Board::Lcd::pclk == Board::Strap::b2, "Korvo LCD PCLK strap mapping changed");
    static_assert(Board::Lcd::mosi == Board::Strap::b3, "Korvo LCD MOSI strap mapping changed");
    static_assert(Board::Lcd::sck == Board::Strap::b4, "Korvo LCD SCK strap mapping changed");
    static_assert(sizeof(Board::Resource::LcdBus) > 0U);

    std::array<std::uint16_t, lcd_probe.size()> frame{};
    for (std::size_t i = 0; i < frame.size(); ++i) {
        frame[i] = lcd_probe[i];
    }

    arc::DmaChain<1> chain{};
    static_cast<void>(chain.try_bind(0, std::span<std::uint16_t>{frame}, true));

    std::printf(
        "arc-s31-lcd target=%s board=%s arch=%s display=%s panel=%s touch=%s inch_x10=%u res=%ux%u rgb565=%d lcd=%d..%d sync=%d,%d,%d,%d ctl=%d,%d,%d lcd_edges=%zu strap_edges=%zu driver_contract=%d frame_pixels=%zu dma_bytes=%zu ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::Display::expansion,
        Board::Display::panel_driver,
        Board::Display::touch_driver,
        static_cast<unsigned>(Board::Display::inch_x10),
        static_cast<unsigned>(Board::Display::hres),
        static_cast<unsigned>(Board::Display::vres),
        Board::Display::rgb565 ? 1 : 0,
        Board::Lcd::db0,
        Board::Lcd::db15,
        Board::Lcd::hsync,
        Board::Lcd::vsync,
        Board::Lcd::hen,
        Board::Lcd::pclk,
        Board::Lcd::cs,
        Board::Lcd::mosi,
        Board::Lcd::sck,
        arc::board::Korvo1LcdGraph::edge_count,
        arc::board::Korvo1StrapGraph::edge_count,
        ARC_S31_KORVO_LCD_DRIVER_CONTRACT,
        frame.size(),
        chain.bytes(),
        Board::Onboard::lcd ? 1 : 0);
}
