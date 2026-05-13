#pragma once

#include <cstdint>

#include "arc/arch/xtensa.hpp"

namespace arc::soc {

struct Esp32S3 {
    using Arch = arch::Xtensa;

    static constexpr const char* name = "esp32s3";
    static constexpr const char* arch = Arch::name;
    static constexpr std::uint32_t cores = 2U;
    static constexpr std::uint32_t mhz = 240U;

    static constexpr bool stable = true;
    static constexpr bool experimental = false;
    static constexpr bool simd = true;
    static constexpr bool wifi = true;
    static constexpr bool wifi6 = false;
    static constexpr bool ble = true;
    static constexpr bool ieee802154 = false;
    static constexpr bool ethernet_mac = false;
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
    static constexpr bool mask = true;
    static constexpr bool trax = true;

    struct Api {
        static constexpr bool drive = true;
        static constexpr bool sense = true;
        static constexpr bool app = true;
        static constexpr bool tight = true;
        static constexpr bool ptp = false;
        static constexpr bool ml = true;
        static constexpr bool cache = true;
        static constexpr bool dma = true;
        static constexpr bool tee = true;
        static constexpr bool world = true;
        static constexpr bool amp = false;
        static constexpr bool cam = true;
        static constexpr bool control = true;
    };
};

}  // namespace arc::soc
