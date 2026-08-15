#include <cstdint>
#include <cstdio>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/ml.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

extern "C" void app_main()
{
    using Board = arc::board::Korvo1;

    static_assert(arc::soc::s31, "ml requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);
    static_assert(arc::soc::Target::simd, "ESP32-S31 ML scaffold expects SIMD capability");
    static_assert(arc::Soc::simd, "ESP32-S31 ML scaffold expects SDK SIMD capability");

    const auto q = arc::ml::saturate_s8(130);
    std::printf(
        "arc-s31-ml target=%s board=%s arch=%s simd=%d ready=%d q=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        arc::soc::Target::simd ? 1 : 0,
        arc::soc::has<arc::soc::Cap::ml> ? 1 : 0,
        static_cast<int>(q));
}
