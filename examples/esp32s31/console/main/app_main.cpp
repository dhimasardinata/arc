#include <cstdio>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("driver/uart.h")
#include "arc/uart.hpp"
#define ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

#if ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT
using Console = arc::Uart<UART_NUM_0, Board::Uart0::tx, Board::Uart0::rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 115'200, 256>;

static_assert(Console::tx() == Board::Uart0::tx);
static_assert(Console::rx() == Board::Uart0::rx);
static_assert(sizeof(Console::Resource) > 0U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "console requires ARC_TARGET=esp32s31");
    static_assert(arc::Soc::uart, "ESP32-S31 console scaffold expects SDK UART capability");
    static_assert(arc::Topology<Board>);
    static_assert(arc::board::Korvo1ConsoleGraph::valid());
    static_assert(Board::Uart0::tx == 58 && Board::Uart0::rx == 59, "Korvo UART0 pins changed");
    static_assert(Board::ConsoleBridge::usb_c, "Korvo console bridge USB-C fact changed");
    static_assert(Board::ConsoleBridge::powers_board, "Korvo console bridge power fact changed");
    static_assert(Board::ConsoleBridge::flash, "Korvo console bridge flash fact changed");
    static_assert(Board::ConsoleBridge::max_baud == 3'000'000U, "Korvo console bridge baud limit changed");
    static_assert(Board::Power::uart_power, "Korvo UART USB-C power fact changed");
    static_assert(Board::Download::uart, "Korvo UART download path changed");
    static_assert(Board::Download::manual, "Korvo manual download path changed");
    static_assert(Board::Download::auto_download, "Korvo automatic download path changed");
    static_assert(Board::Download::dtr_rts, "Korvo DTR/RTS download controls changed");
    static_assert(Board::Download::boot_btn && Board::Download::rst_btn, "Korvo manual download buttons changed");
    static_assert(Board::Setup::usb_cables == 2U, "Korvo setup USB cable count changed");
    static_assert(Board::Setup::usb2 && Board::Setup::a_to_c, "Korvo setup USB cable type changed");
    static_assert(Board::Setup::data_cable, "Korvo programming cable data-line requirement changed");
    static_assert(Board::Setup::switch_on && Board::Setup::red_led, "Korvo power setup indicator changed");
    static_assert(sizeof(Board::Resource::ConsoleUart) > 0U);

    std::printf(
        "arc-s31-console target=%s board=%s arch=%s uart0=%d,%d bridge_usb_c=%d bridge_power=%d bridge_flash=%d bridge_max_baud=%u uart_power=%d manual_dl=%d auto_dl=%d dtr_rts=%d setup_usb_cables=%u setup_data_cable=%d red_led=%d console_edges=%zu driver_contract=%d ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::Uart0::tx,
        Board::Uart0::rx,
        Board::ConsoleBridge::usb_c ? 1 : 0,
        Board::ConsoleBridge::powers_board ? 1 : 0,
        Board::ConsoleBridge::flash ? 1 : 0,
        static_cast<unsigned>(Board::ConsoleBridge::max_baud),
        Board::Power::uart_power ? 1 : 0,
        Board::Download::manual ? 1 : 0,
        Board::Download::auto_download ? 1 : 0,
        Board::Download::dtr_rts ? 1 : 0,
        static_cast<unsigned>(Board::Setup::usb_cables),
        Board::Setup::data_cable ? 1 : 0,
        Board::Setup::red_led ? 1 : 0,
        arc::board::Korvo1ConsoleGraph::edge_count,
        ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT,
        Board::pins::has<Board::Uart0::tx>() && Board::pins::has<Board::Uart0::rx>() ? 1 : 0);
}
