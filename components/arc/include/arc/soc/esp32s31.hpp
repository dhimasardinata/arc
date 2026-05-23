#pragma once

#include <cstdint>

#include "arc/arch/riscv.hpp"

namespace arc::soc {

struct Esp32S31 {
    using Arch = arch::Riscv;

    static constexpr const char* name = "esp32s31";
    static constexpr const char* arch = Arch::name;
    static constexpr std::uint32_t cores = 2U;
    static constexpr std::uint32_t mhz = 0U;

    static constexpr bool stable = false;
    static constexpr bool experimental = true;
    static constexpr bool simd = true;
    static constexpr bool wifi = true;
    static constexpr bool wifi6 = true;
    static constexpr bool ble = true;
    static constexpr bool ieee802154 = true;
    static constexpr bool ethernet_mac = true;
    static constexpr bool usb_otg = true;
    static constexpr bool camera = true;
    static constexpr bool display = true;

    static constexpr bool secure_boot = true;
    static constexpr bool flash_encryption = true;
    static constexpr bool tee = true;
    static constexpr bool puf = true;
    static constexpr bool worldguard = true;

    static constexpr bool dedicated_gpio = false;
    static constexpr bool gdma = false;
    static constexpr bool dma2d = false;
    static constexpr bool ppa = false;
    static constexpr bool jpeg = false;
    static constexpr bool h264 = false;
    static constexpr bool mask = false;
    static constexpr bool trax = false;

    struct Api {
        static constexpr bool drive = false;
        static constexpr bool sense = false;
        static constexpr bool app = true;
        static constexpr bool tight = true;
        static constexpr bool ptp = false;
        static constexpr bool ml = false;
        static constexpr bool cache = true;
        static constexpr bool dma = true;
        static constexpr bool tee = true;
        static constexpr bool world = true;
        static constexpr bool amp = false;
        static constexpr bool cam = false;
        static constexpr bool control = false;
    };
};

}  // namespace arc::soc
