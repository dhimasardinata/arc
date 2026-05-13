#include <array>
#include <cstdint>
#include <cstdio>

#include "arc.hpp"

namespace app {

inline constexpr std::uint16_t eeprom = 0x50;

using Bus = arc::I2cBus<0, 8, 9>;
using Dev = arc::I2c<Bus, eeprom, 400'000>;

inline void boot()
{
    const auto bus = Bus::init();
    std::printf("arc-i2c bus init=0x%x sda=%d scl=%d\n", static_cast<unsigned>(bus), Bus::sda(), Bus::scl());

    const auto seen = Bus::probe(eeprom, 50);
    std::printf("arc-i2c probe addr=0x%02x ret=0x%x\n", eeprom, static_cast<unsigned>(seen));

    std::array<std::uint8_t, 1> reg{0U};
    std::array<std::uint8_t, 8> rx{};
    const auto xfer = Dev::xfer(reg.data(), reg.size(), rx.data(), rx.size(), 50);
    std::printf("arc-i2c xfer ret=0x%x first=0x%02x\n", static_cast<unsigned>(xfer), rx[0]);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
