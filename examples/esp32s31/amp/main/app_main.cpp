#include <cstdio>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc/target.hpp"
#include "arc/task.hpp"

extern "C" void app_main()
{
    using Board = arc::board::Korvo1;
    using Map = arc::CoreMap<>;

    static_assert(arc::soc::s31, "amp requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);
    static_assert(Map::ctrl == arc::Core::core0, "Core0 is the Arc control core");
    static_assert(Map::det == arc::Core::core1, "Core1 is the Arc deterministic core");
    static_assert(!arc::soc::has<arc::soc::Cap::amp>, "ESP32-S31 true bare-core AMP policy is not wired yet");

    std::printf(
        "arc-s31-amp target=%s board=%s arch=%s riscv=%d simd=%d bare_core=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Map::riscv ? 1 : 0,
        Map::simd ? 1 : 0,
        arc::soc::has<arc::soc::Cap::amp> ? 1 : 0);
}
