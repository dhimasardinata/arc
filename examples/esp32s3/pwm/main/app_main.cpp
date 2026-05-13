#include <cstdint>

#include "arc.hpp"
#include "sdkconfig.h"

using Led = arc::Pwm<
    CONFIG_ARC_PWM_LED,
    static_cast<std::uint32_t>(CONFIG_ARC_PWM_HZ),
    static_cast<std::uint32_t>(CONFIG_ARC_PWM_DUTY)>;

namespace app {

inline void boot()
{
    Led::start();
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
