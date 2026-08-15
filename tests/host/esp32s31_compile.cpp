#define ARC_TARGET_ESP32S31 1

#include "arc.hpp"
#include "arc/dma_chain.hpp"
#include "arc/interrupt_matrix.hpp"
#include "arc/mask.hpp"
#include "arc/ml.hpp"
#include "arc/soc/target.hpp"
#include "arc/task.hpp"
#include "arc/tee.hpp"
#include "arc/timesync.hpp"
#include "arc/touch.hpp"

namespace {

using Board = arc::board::Korvo1;
using Target = arc::soc::Target;
using Map = arc::CoreMap<>;

struct S31App {
    static void setup() {}
    static void loop() noexcept {}
};

struct S31Loop {
    static void setup() {}
    static void step() noexcept {}
};

using App = arc::App<S31App, 2048U, Map::det>;
using Control = arc::Tight<S31Loop, 2048U, Map::det>;

void s31_irq_handler() noexcept {}
using S31Irq = arc::InterruptMatrix<17, 4, 1, s31_irq_handler>;

static_assert(ARC_TARGET_IS_ESP32S31 == 1);
static_assert(ARC_TARGET_ARCH_RISCV == 1);
static_assert(ARC_TARGET_ARCH_XTENSA == 0);
static_assert(arc::soc::s31);
static_assert(!arc::soc::s3);
static_assert(!arc::soc::p4);
static_assert(Target::experimental);
static_assert(Target::cores == 2U);
static_assert(Target::Arch::csr);
static_assert(Target::wifi6);
static_assert(Target::bt54);
static_assert(Target::bt_classic);
static_assert(Target::ieee802154);
static_assert(Target::ethernet_mac);
static_assert(Target::camera);
static_assert(Target::display);
static_assert(Target::secure_boot);
static_assert(Target::flash_encryption);
static_assert(Target::tee);
static_assert(Target::puf);
static_assert(Target::worldguard);
static_assert(!Target::trax);
static_assert(arc::Soc::experimental);
static_assert(arc::Soc::cores == 2U);
static_assert(!arc::Soc::fast_gpio);
static_assert(!arc::Soc::gdma);
static_assert(!arc::Soc::ahb_dma);
static_assert(arc::Soc::i2c);
static_assert(arc::Soc::i2s);
static_assert(arc::Soc::dvp);
static_assert(arc::Soc::lcd_i80);
static_assert(arc::Soc::lcd_rgb);
static_assert(arc::Soc::uart);
static_assert(arc::Soc::simd);
static_assert(arc::Soc::adc);
static_assert(arc::Soc::rmt);
static_assert(arc::Soc::sdmmc);
static_assert(arc::Soc::usb_otg);
static_assert(!arc::Soc::touch);
static_assert(arc::Soc::wifi);
static_assert(arc::Soc::ble);
static_assert(arc::Soc::ble_mesh);
static_assert(arc::Soc::gpio_pins == 64U);
static_assert(arc::Soc::gpio_out == 61U);
static_assert(arc::Soc::touch_max == 0U);
static_assert(!arc::Soc::mpi);
static_assert(Map::dual);
static_assert(Map::riscv);
static_assert(Map::det == arc::Core::core1);
static_assert(Map::ctrl == arc::Core::core0);
static_assert(arc::soc::has<arc::soc::Cap::ptp>);
static_assert(arc::soc::has<arc::soc::Cap::ml>);
static_assert(arc::soc::has<arc::soc::Cap::cam>);
static_assert(arc::soc::has<arc::soc::Cap::control>);
static_assert(arc::soc::has<arc::soc::Cap::tee>);
static_assert(arc::soc::has<arc::soc::Cap::world>);
static_assert(!arc::soc::has<arc::soc::Cap::amp>);
static_assert(App::stack_bytes == 2048U);
static_assert(Control::stack_bytes == 2048U);
static_assert(sizeof(arc::Mask<1>) > 0U);
static_assert(sizeof(arc::Critical) > 0U);
static_assert(sizeof(arc::Silence) > 0U);
static_assert(arc::cache_line == 64U);
static_assert(S31Irq::binding.source == 17);
static_assert(S31Irq::binding.cpu_interrupt == 4U);
static_assert(S31Irq::binding.level == 1U);
static_assert(arc::DmaChain<2>{}.size() == 2U);
static_assert(arc::ml::saturate_s8(130) == 127);
static_assert(arc::ml::saturate_s8(-130) == -128);
static_assert(arc::PtpConfig{}.step_max == 1'000'000);
static_assert(arc::TeePlan{}.trusted_core == 1U);
static_assert(arc::TeePlan{}.untrusted_core == 0U);
static_assert(arc::Topology<Board>);
static_assert(Board::flash_mb == 16U);
static_assert(Board::psram_mb == 16U);
static_assert(Board::pins::count == 54U);
static_assert(Board::pins::has<Board::Audio::mclk>());
static_assert(Board::pins::has<Board::Lcd::sck>());
static_assert(Board::pins::has<Board::Cam::hsync>());
static_assert(!Board::pins::has<29>());
static_assert(!Board::pins::has<41>());

}  // namespace
