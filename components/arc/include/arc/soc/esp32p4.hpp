#pragma once

#include <cstdint>

#include "arc/arch/riscv.hpp"

namespace arc::soc {

struct Esp32P4 {
    using Arch = arch::Riscv;

    static constexpr const char* name = "esp32p4";
    static constexpr const char* arch = Arch::name;
    static constexpr std::uint32_t cores = 2U;
    static constexpr std::uint32_t mhz = 400U;

    static constexpr bool stable = true;
    static constexpr bool experimental = false;
    static constexpr bool simd = true;
    static constexpr bool wifi = false;
    static constexpr bool wifi6 = false;
    static constexpr bool ble = false;
    static constexpr bool ieee802154 = false;
    static constexpr bool ethernet_mac = true;
    static constexpr bool usb_otg = true;
    static constexpr bool camera = true;
    static constexpr bool display = true;

    static constexpr bool secure_boot = true;
    static constexpr bool flash_encryption = true;
    static constexpr bool tee = false;
    static constexpr bool puf = false;
    static constexpr bool worldguard = false;

    static constexpr bool dedicated_gpio = true;
    static constexpr bool gdma = true;
    static constexpr bool dma2d = true;
    static constexpr bool mipi_csi = true;
    static constexpr bool mipi_dsi = true;
    static constexpr bool jpeg = true;
    static constexpr bool mask = false;
    static constexpr bool trax = false;

    struct Api {
        static constexpr bool drive = false;
        static constexpr bool sense = false;
        static constexpr bool app = true;
        static constexpr bool tight = true;
        static constexpr bool ptp = true;
        static constexpr bool ml = true;
        static constexpr bool cache = true;
        static constexpr bool dma = true;
        static constexpr bool tee = false;
        static constexpr bool world = false;
        static constexpr bool amp = false;
        static constexpr bool cam = true;
        static constexpr bool control = true;
    };
};

}  // namespace arc::soc
