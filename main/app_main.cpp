#include "arc.hpp"

using Sense = arc::Din<arc::cfg::sense, 0>;
using Led = arc::Dio<arc::cfg::led, 0>;

struct Mirror {
    static void setup()
    {
        Sense::in();
        Led::out();
        Led::template write<false>();
    }

    IRAM_ATTR static void loop() noexcept
    {
        if (Sense::high()) {
            Led::hi();
        } else {
            Led::lo();
        }
    }
};

using Core1 = arc::Sketch<Mirror, arc::cfg::stack, arc::Core::core1>;

namespace app {

inline void boot()
{
    Core1::boot("arc");
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
