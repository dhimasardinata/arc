#include <cstdint>
#include <cstdio>

#include "arc/soc/target.hpp"
#include "arc/timesync.hpp"

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "ptp requires ARC_TARGET=esp32s31");
    static_assert(arc::soc::Target::ethernet_mac, "ESP32-S31 PTP scaffold expects Ethernet MAC capability");

    arc::PtpClock clock{};
    const auto stats = clock.discipline(arc::PtpSample{
        .origin_ns = 1'000'000,
        .ingress_ns = 1'000'120,
        .egress_ns = 1'000'180,
        .receive_ns = 1'000'320,
    });

    std::printf(
        "arc-s31-ptp target=%s arch=%s ready=%d offset_ns=%lld samples=%lu\n",
        arc::soc::name,
        arc::soc::arch,
        arc::soc::has<arc::soc::Cap::ptp> ? 1 : 0,
        static_cast<long long>(clock.offset_ns),
        static_cast<unsigned long>(stats.samples));
}
