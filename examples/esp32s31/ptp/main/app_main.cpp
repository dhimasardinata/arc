#include <cstdint>
#include <cstdio>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc/target.hpp"
#include "arc/timesync.hpp"

extern "C" void app_main()
{
    using Board = arc::board::Korvo1;

    static_assert(arc::soc::s31, "ptp requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);
    static_assert(arc::soc::Target::ethernet_mac, "ESP32-S31 PTP scaffold expects Ethernet MAC capability");
    static_assert(!Board::Onboard::eth_phy, "Korvo PTP needs an external Ethernet PHY path");

    arc::PtpClock clock{};
    const auto stats = clock.discipline(arc::PtpSample{
        .origin_ns = 1'000'000,
        .ingress_ns = 1'000'120,
        .egress_ns = 1'000'180,
        .receive_ns = 1'000'320,
    });

    std::printf(
        "arc-s31-ptp target=%s board=%s arch=%s ready=%d external_phy=%d offset_ns=%lld samples=%lu\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        arc::soc::has<arc::soc::Cap::ptp> ? 1 : 0,
        Board::Onboard::eth_phy ? 0 : 1,
        static_cast<long long>(clock.offset_ns),
        static_cast<unsigned long>(stats.samples));
}
