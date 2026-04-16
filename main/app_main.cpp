#include "arc.hpp"

using Button = arc::Sense<arc::cfg::sense, 0>;
using Led = arc::Drive<arc::cfg::led, 0>;

struct Follow {
    static void setup()
    {
        Button::in();
        Led::out();
        Led::off();
    }

    IRAM_ATTR static void loop() noexcept
    {
        if (Button::high()) {
            Led::on();
        } else {
            Led::off();
        }
    }
};

using Core1 = arc::App<Follow, arc::cfg::stack, arc::Core::core1>;

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
