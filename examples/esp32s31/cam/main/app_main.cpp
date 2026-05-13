#include <array>
#include <cstdint>
#include <cstdio>
#include <span>

#include "arc/dma_chain.hpp"
#include "arc/soc/target.hpp"

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "cam requires ARC_TARGET=esp32s31");
    static_assert(arc::soc::Target::camera && arc::soc::Target::display, "ESP32-S31 scaffold expects camera/display capability");

    std::array<std::uint8_t, 64> line{};
    arc::DmaChain<2> chain{};
    chain.bind(0, std::span<std::uint8_t>{line});
    chain.link_circular();

    std::printf(
        "arc-s31-cam target=%s arch=%s dma_head=%p ready=%d\n",
        arc::soc::name,
        arc::soc::arch,
        static_cast<void*>(chain.head()),
        arc::soc::has<arc::soc::Cap::cam> ? 1 : 0);
}
