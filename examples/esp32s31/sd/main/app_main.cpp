#include <cstdint>
#include <cstdio>
#include <string_view>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("driver/gpio.h") && __has_include("driver/sdmmc_host.h") && __has_include("esp_vfs_fat.h") && __has_include("sdmmc_cmd.h")
#include "arc/gpio.hpp"
#include "arc/sd.hpp"
#define ARC_S31_KORVO_SD_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_SD_DRIVER_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

#if ARC_S31_KORVO_SD_DRIVER_CONTRACT
using CardCtrl = arc::Gpio<Board::Sd::ctrl>;
using Storage = arc::Sd<
    Board::Sd::clk,
    Board::Sd::cmd,
    Board::Sd::d0,
    Board::Sd::d1,
    Board::Sd::d2,
    Board::Sd::d3,
    4>;

static_assert(CardCtrl::mask() == (std::uint32_t{1} << (Board::Sd::ctrl - 32)));
static_assert(Storage::width() == 4U);
static_assert(sizeof(Board::Resource::SdSlot) > 0U);
static_assert(sizeof(Storage::Resource) > 0U);
static_assert(sizeof(Board::Resource::SdCtrl) > 0U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "sd requires ARC_TARGET=esp32s31");
    static_assert(arc::Soc::sdmmc, "ESP32-S31 SD scaffold expects SDK SDMMC capability");
    static_assert(arc::Topology<Board>);
    static_assert(arc::board::Korvo1SdGraph::valid());
    static_assert(arc::board::Korvo1NandGraph::valid());
    static_assert(arc::board::Korvo1StrapGraph::valid());
    static_assert(Board::Onboard::microsd, "Korvo SD example requires the onboard microSD lane");
    static_assert(Board::Sd::width == 4U, "Korvo microSD bus width changed");
    static_assert(Board::Sd::sdio3, "Korvo microSD SDIO capability changed");
    static_assert(Board::Sd::audio_store && Board::Sd::playback, "Korvo microSD audio role changed");
    static_assert(!Board::Onboard::spi_nand, "Korvo SPI NAND is not populated by default");
    static_assert(Board::SpiNand::shares_sd, "Korvo SPI NAND no longer shares the SD lane");
    static_assert(Board::SpiNand::requires_rework, "Korvo SPI NAND rework contract changed");
    static_assert(Board::SpiNand::supports_1v8 && Board::SpiNand::supports_3v3, "Korvo SPI NAND voltage support changed");
    static_assert(std::string_view{Board::SpiNand::remove} == "R7,R65,R66,R67,R68,R69", "Korvo NAND removal list changed");
    static_assert(std::string_view{Board::SpiNand::base_pop} == "R22,R23,R1,R2,R3,R4,C6,R20,U4", "Korvo NAND base population list changed");
    static_assert(std::string_view{Board::SpiNand::v18_pop} == "R134,C66,C80,R100,U1,C82,C67", "Korvo NAND 1.8 V population list changed");
    static_assert(std::string_view{Board::SpiNand::v33_pop} == "R135", "Korvo NAND 3.3 V population list changed");
    static_assert(Board::SpiNand::remove_count == 6U && Board::SpiNand::base_count == 9U, "Korvo NAND base rework count changed");
    static_assert(Board::SpiNand::v18_count == 7U && Board::SpiNand::v33_count == 1U, "Korvo NAND voltage rework count changed");
    static_assert(Board::Sd::d0 == 20 && Board::Sd::d3 == 23, "Korvo SD data bus changed");
    static_assert(Board::Sd::clk == 24 && Board::Sd::cmd == 25, "Korvo SD control pins changed");
    static_assert(Board::Sd::ctrl == 39, "Korvo SD control GPIO changed");
    static_assert(Board::Sd::d0 == Board::SpiNand::clk, "Korvo SD/NAND GPIO20 alias changed");
    static_assert(Board::Sd::d1 == Board::SpiNand::d, "Korvo SD/NAND GPIO21 alias changed");
    static_assert(Board::Sd::d2 == Board::SpiNand::q, "Korvo SD/NAND GPIO22 alias changed");
    static_assert(Board::Sd::d3 == Board::SpiNand::cs, "Korvo SD/NAND GPIO23 alias changed");
    static_assert(Board::Sd::clk == Board::SpiNand::hold, "Korvo SD/NAND GPIO24 alias changed");
    static_assert(Board::Sd::cmd == Board::SpiNand::wp, "Korvo SD/NAND GPIO25 alias changed");
    static_assert(Board::Sd::ctrl == Board::Strap::b1, "Korvo SD control strap mapping changed");

    std::printf(
        "arc-s31-sd target=%s board=%s arch=%s sd=%d,%d,%d,%d,%d,%d width=%u sdio3=%d audio_store=%d playback=%d ctrl=%d nand_shared=%d nand_connected=%d nand_rework=%d nand_remove=%u nand_base=%u nand_1v8=%d nand_1v8_parts=%u nand_3v3=%d nand_3v3_parts=%u sd_edges=%zu nand_edges=%zu strap_edges=%zu driver_contract=%d ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::Sd::clk,
        Board::Sd::cmd,
        Board::Sd::d0,
        Board::Sd::d1,
        Board::Sd::d2,
        Board::Sd::d3,
        static_cast<unsigned>(Board::Sd::width),
        Board::Sd::sdio3 ? 1 : 0,
        Board::Sd::audio_store ? 1 : 0,
        Board::Sd::playback ? 1 : 0,
        Board::Sd::ctrl,
        Board::SpiNand::shares_sd ? 1 : 0,
        Board::SpiNand::connected ? 1 : 0,
        Board::SpiNand::requires_rework ? 1 : 0,
        static_cast<unsigned>(Board::SpiNand::remove_count),
        static_cast<unsigned>(Board::SpiNand::base_count),
        Board::SpiNand::supports_1v8 ? 1 : 0,
        static_cast<unsigned>(Board::SpiNand::v18_count),
        Board::SpiNand::supports_3v3 ? 1 : 0,
        static_cast<unsigned>(Board::SpiNand::v33_count),
        arc::board::Korvo1SdGraph::edge_count,
        arc::board::Korvo1NandGraph::edge_count,
        arc::board::Korvo1StrapGraph::edge_count,
        ARC_S31_KORVO_SD_DRIVER_CONTRACT,
        Board::Onboard::microsd ? 1 : 0);
}
