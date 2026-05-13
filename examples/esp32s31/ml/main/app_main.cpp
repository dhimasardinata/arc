#include <cstdint>
#include <cstdio>

#include "arc/ml.hpp"
#include "arc/soc/target.hpp"

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "ml requires ARC_TARGET=esp32s31");
    static_assert(arc::soc::Target::simd, "ESP32-S31 ML scaffold expects SIMD capability");

    const auto q = arc::ml::saturate_s8(130);
    std::printf(
        "arc-s31-ml target=%s arch=%s simd=%d ready=%d q=%d\n",
        arc::soc::name,
        arc::soc::arch,
        arc::soc::Target::simd ? 1 : 0,
        arc::soc::has<arc::soc::Cap::ml> ? 1 : 0,
        static_cast<int>(q));
}
