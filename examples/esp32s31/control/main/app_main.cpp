#include <cstdio>

#include "arc/soc/target.hpp"
#include "arc/tight.hpp"

namespace {

struct ControlLoop {
    static void setup() {}
    static void step() noexcept {}
};

using Control = arc::Tight<ControlLoop, 2048U, arc::CoreMap<>::det>;

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "control requires ARC_TARGET=esp32s31");
    static_assert(Control::stack_bytes == 2048U, "Control loop stack budget should stay explicit");

    std::printf(
        "arc-s31-control target=%s arch=%s bind=%ld ready=%d\n",
        arc::soc::name,
        arc::soc::arch,
        static_cast<long>(arc::CoreMap<>::det),
        arc::soc::has<arc::soc::Cap::control> ? 1 : 0);
}
