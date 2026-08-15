#pragma once

#include <cstdint>

#include "arc/claim.hpp"
#include "arc/topology.hpp"

namespace arc::board {

struct Korvo1 {
    static constexpr const char* name = "esp32-s31-korvo-1";
    static constexpr const char* revision = "v1.1";
    static constexpr std::uint32_t flash_mb = 16U;
    static constexpr std::uint32_t psram_mb = 16U;

    struct Module {
        static constexpr const char* model = "ESP32-S31-WROOM-3";
        static constexpr std::uint32_t flash_mb = 16U;
        static constexpr std::uint32_t psram_mb = 16U;
        static constexpr bool pcb_antenna = true;
    };

    struct Wireless {
        static constexpr bool wifi = true;
        static constexpr bool wifi6 = true;
        static constexpr bool ble = true;
        static constexpr bool bt54 = true;
        static constexpr bool bt_classic = true;
        static constexpr bool ieee802154 = true;
        static constexpr bool zigbee3 = true;
        static constexpr bool thread14 = true;
        static constexpr bool pcb_antenna = Module::pcb_antenna;
    };

    struct Onboard {
        static constexpr bool audio = true;
        static constexpr bool lcd = true;
        static constexpr bool camera = true;
        static constexpr bool microsd = true;
        static constexpr bool button = true;
        static constexpr bool status_led = true;
        static constexpr bool spi_nand = false;
        static constexpr bool usb_otg = true;
        static constexpr bool eth_phy = false;
    };

    struct Audio {
        static constexpr int mclk = 2;
        static constexpr int sclk = 3;
        static constexpr int lrclk = 4;
        static constexpr int dsin = 5;
        static constexpr int sdout = 6;
        static constexpr int pa = 7;
    };

    struct AudioCodec {
        static constexpr const char* model = "ES8389";
        static constexpr const char* pa_model = "NS4150B";
        static constexpr std::uint32_t pa_count = 2U;
        static constexpr std::uint32_t mic_count = 2U;
        static constexpr std::uint32_t speaker_count = 2U;
        static constexpr std::uint32_t speaker_ohm = 4U;
        static constexpr std::uint32_t speaker_w = 3U;
        static constexpr std::uint32_t pitch_mm = 2U;
        static constexpr std::uint32_t pitch_mil = 80U;
        static constexpr bool stereo = true;
        static constexpr bool dual_adc = true;
        static constexpr bool dual_dac = true;
        static constexpr bool analog_mics = true;
        static constexpr bool speech = true;
        static constexpr bool near_wake = true;
        static constexpr bool far_wake = true;
    };

    struct I2c {
        static constexpr int sda = 0;
        static constexpr int scl = 1;
    };

    struct Lcd {
        static constexpr int db0 = 8;
        static constexpr int db1 = 9;
        static constexpr int db2 = 10;
        static constexpr int db3 = 11;
        static constexpr int db4 = 12;
        static constexpr int db5 = 13;
        static constexpr int db6 = 14;
        static constexpr int db7 = 15;
        static constexpr int db8 = 16;
        static constexpr int db9 = 17;
        static constexpr int db10 = 18;
        static constexpr int db11 = 19;
        static constexpr int db12 = 33;
        static constexpr int db13 = 34;
        static constexpr int db14 = 35;
        static constexpr int db15 = 36;
        static constexpr int cs = 38;
        static constexpr int pclk = 40;
        static constexpr int hen = 43;
        static constexpr int hsync = 44;
        static constexpr int vsync = 45;
        static constexpr int mosi = 60;
        static constexpr int sck = 61;
    };

    struct Display {
        static constexpr bool external = true;
        static constexpr bool connector = true;
        static constexpr const char* expansion = "ESP32-S3-LCD-EV-Board-SUB3";
        static constexpr const char* panel_driver = "ST7262E43";
        static constexpr const char* touch_driver = "GT1151";
        static constexpr std::uint32_t inch_x10 = 43U;
        static constexpr std::uint32_t hres = 800U;
        static constexpr std::uint32_t vres = 480U;
        static constexpr bool rgb = true;
        static constexpr bool rgb565 = true;
        static constexpr bool touch = true;
    };

    struct Sd {
        static constexpr std::uint32_t width = 4U;
        static constexpr bool sdio3 = true;
        static constexpr bool audio_store = true;
        static constexpr bool playback = true;
        static constexpr int d0 = 20;
        static constexpr int d1 = 21;
        static constexpr int d2 = 22;
        static constexpr int d3 = 23;
        static constexpr int clk = 24;
        static constexpr int cmd = 25;
        static constexpr int ctrl = 39;
    };

    struct SpiNand {
        static constexpr bool connected = false;
        static constexpr bool shares_sd = true;
        static constexpr bool requires_rework = true;
        static constexpr bool supports_1v8 = true;
        static constexpr bool supports_3v3 = true;
        static constexpr const char* remove = "R7,R65,R66,R67,R68,R69";
        static constexpr const char* base_pop = "R22,R23,R1,R2,R3,R4,C6,R20,U4";
        static constexpr const char* v18_pop = "R134,C66,C80,R100,U1,C82,C67";
        static constexpr const char* v33_pop = "R135";
        static constexpr std::uint32_t remove_count = 6U;
        static constexpr std::uint32_t base_count = 9U;
        static constexpr std::uint32_t v18_count = 7U;
        static constexpr std::uint32_t v33_count = 1U;
        static constexpr int clk = 20;
        static constexpr int d = 21;
        static constexpr int q = 22;
        static constexpr int cs = 23;
        static constexpr int hold = 24;
        static constexpr int wp = 25;
    };

    struct Cam {
        static constexpr int d0 = 46;
        static constexpr int d1 = 47;
        static constexpr int d2 = 48;
        static constexpr int d3 = 49;
        static constexpr int d4 = 50;
        static constexpr int d5 = 51;
        static constexpr int d6 = 52;
        static constexpr int d7 = 53;
        static constexpr int pclk = 54;
        static constexpr int xclk = 55;
        static constexpr int vsync = 56;
        static constexpr int hsync = 57;
    };

    struct CamModule {
        static constexpr bool external = true;
        static constexpr bool connector = true;
        static constexpr const char* model = "OV3660";
        static constexpr std::uint32_t ldo_in_mv = 3300U;
        static constexpr std::uint32_t avdd_mv = 2800U;
        static constexpr std::uint32_t dvdd_mv = 1500U;
        static constexpr bool avdd_ldo = true;
        static constexpr bool dvdd_ldo = true;
        static constexpr bool video_stream = true;
        static constexpr bool jpeg_stream = true;
    };

    struct Uart0 {
        static constexpr int tx = 58;
        static constexpr int rx = 59;
    };

    struct ConsoleBridge {
        static constexpr bool usb_c = true;
        static constexpr bool powers_board = true;
        static constexpr bool flash = true;
        static constexpr std::uint32_t max_baud = 3'000'000U;
    };

    struct Download {
        static constexpr bool uart = true;
        static constexpr bool manual = true;
        static constexpr bool auto_download = true;
        static constexpr bool dtr_rts = true;
        static constexpr bool boot_btn = true;
        static constexpr bool rst_btn = true;
    };

    struct Button {
        static constexpr int adc = 42;
        static constexpr const char* signal = "ADC BUTTON";
        static constexpr int count = 4;
        static constexpr int play = 0;
        static constexpr int set = 1;
        static constexpr int vol_down = 2;
        static constexpr int vol_up = 3;
        static constexpr bool shared_adc = true;
        static constexpr bool ui_control = true;
        static constexpr bool audio_test = true;
    };

    struct Led {
        // Espressif's V1.1 component overview says GPIO8, but the pin table
        // maps GPIO8 to LCD DB0 and WS2812_CTRL to GPIO37.
        static constexpr int ws2812 = 37;
        static constexpr const char* signal = "WS2812_CTRL";
        static constexpr std::uint32_t count = 1U;
        static constexpr bool rgb = true;
        static constexpr bool addressable = true;
    };

    struct Usb {
        // These are ESP32-S31-WROOM-3 module pins, not GPIO numbers.
        static constexpr int dp_module_pin = 40;
        static constexpr int dm_module_pin = 41;
        static constexpr int dp_gpio = -1;
        static constexpr int dm_gpio = -1;
        static constexpr bool module_pins_are_gpio = false;
    };

    struct UsbHost {
        static constexpr bool type_a = true;
        static constexpr bool high_speed = true;
        static constexpr bool downstream_power = true;
        static constexpr bool current_limited = true;
        static constexpr const char* switch_model = "TPS2051C";
        static constexpr std::uint32_t downstream_ma = 500U;
        static constexpr std::uint32_t port = 1U;
    };

    struct Power {
        static constexpr bool power_only = true;
        static constexpr bool uart_power = true;
        static constexpr bool switch_5v = true;
        static constexpr bool audio_split = true;
        static constexpr bool buck_3v3 = true;
        static constexpr bool audio_ldo_3v3 = true;
        static constexpr bool power_led_5v = true;
        static constexpr std::uint32_t input_ma = 3'000U;
    };

    struct Setup {
        static constexpr std::uint32_t usb_cables = 2U;
        static constexpr bool usb2 = true;
        static constexpr bool a_to_c = true;
        static constexpr bool data_cable = true;
        static constexpr std::uint32_t speaker_min = 1U;
        static constexpr std::uint32_t speaker_max = 2U;
        static constexpr bool switch_on = true;
        static constexpr bool red_led = true;
        static constexpr bool microsd_optional = true;
    };

    struct Strap {
        static constexpr int lcd_db15 = 36;
        static constexpr int status_led = 37;
        static constexpr int b0 = 38;
        static constexpr int b1 = 39;
        static constexpr int b2 = 40;
        static constexpr int b3 = 60;
        static constexpr int b4 = 61;
    };

    struct Resource {
        using CodecI2c = ClaimFor<ClaimKind::i2c_bus, 0, I2c::sda, I2c::scl>;
        using AudioBus = ClaimFor<ClaimKind::i2s_bus,
                                  0,
                                  Audio::mclk,
                                  Audio::sclk,
                                  Audio::lrclk,
                                  Audio::dsin,
                                  Audio::sdout,
                                  Audio::pa>;
        using AudioPa = ClaimFor<ClaimKind::gpio_pin, Audio::pa, Audio::pa>;
        using LcdBus = ClaimFor<ClaimKind::lcd_rgb,
                                0,
                                Lcd::db0,
                                Lcd::db1,
                                Lcd::db2,
                                Lcd::db3,
                                Lcd::db4,
                                Lcd::db5,
                                Lcd::db6,
                                Lcd::db7,
                                Lcd::db8,
                                Lcd::db9,
                                Lcd::db10,
                                Lcd::db11,
                                Lcd::db12,
                                Lcd::db13,
                                Lcd::db14,
                                Lcd::db15,
                                Lcd::cs,
                                Lcd::pclk,
                                Lcd::hen,
                                Lcd::hsync,
                                Lcd::vsync,
                                Lcd::mosi,
                                Lcd::sck>;
        using SdNandLane = ClaimFor<ClaimKind::gpio_pin,
                                    Sd::d0,
                                    ClaimKind::sdmmc_slot,
                                    Sd::d0,
                                    Sd::d1,
                                    Sd::d2,
                                    Sd::d3,
                                    Sd::clk,
                                    Sd::cmd>;
        using SdSlot = ClaimSet<ClaimFor<ClaimKind::sdmmc_slot,
                                         1,
                                         Sd::d0,
                                         Sd::d1,
                                         Sd::d2,
                                         Sd::d3,
                                         Sd::clk,
                                         Sd::cmd,
                                         Sd::ctrl>,
                                SdNandLane>;
        using SdCtrl = ClaimFor<ClaimKind::gpio_pin, Sd::ctrl, Sd::ctrl>;
        using SpiNandLane = ClaimFor<ClaimKind::gpio_pin,
                                     Sd::d0,
                                     ClaimKind::spi_bus,
                                     SpiNand::clk,
                                     SpiNand::d,
                                     SpiNand::q,
                                     SpiNand::cs,
                                     SpiNand::hold,
                                     SpiNand::wp>;
        using SpiNandBus = ClaimSet<ClaimFor<ClaimKind::spi_bus,
                                             2,
                                             SpiNand::clk,
                                             SpiNand::d,
                                             SpiNand::q,
                                             SpiNand::cs,
                                             SpiNand::hold,
                                             SpiNand::wp>,
                                    SpiNandLane>;
        using CamBus = ClaimFor<ClaimKind::camera_dvp,
                                0,
                                Cam::d0,
                                Cam::d1,
                                Cam::d2,
                                Cam::d3,
                                Cam::d4,
                                Cam::d5,
                                Cam::d6,
                                Cam::d7,
                                Cam::pclk,
                                Cam::xclk,
                                Cam::vsync,
                                Cam::hsync>;
        using ConsoleUart = ClaimFor<ClaimKind::uart, 0, Uart0::tx, Uart0::rx>;
        using ButtonAdc = ClaimFor<ClaimKind::adc_dev, Button::adc, Button::adc>;
        using StatusLed = ClaimFor<ClaimKind::gpio_pin, Led::ws2812, Led::ws2812>;
        using UsbOtg = ClaimFor<ClaimKind::usb_otg, 0, 0>;
    };

    using pins = arc::Pins<
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        33,
        34,
        35,
        36,
        37,
        38,
        39,
        40,
        42,
        43,
        44,
        45,
        46,
        47,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        58,
        59,
        60,
        61>;
};

static_assert(Topology<Korvo1>);

struct Korvo1Signal {
    static constexpr std::uint32_t codec_i2c = 0x100U;
    static constexpr std::uint32_t audio_clock = 0x200U;
    static constexpr std::uint32_t audio_data = 0x201U;
    static constexpr std::uint32_t audio_pa = 0x202U;
    static constexpr std::uint32_t lcd_data = 0x300U;
    static constexpr std::uint32_t lcd_sync = 0x301U;
    static constexpr std::uint32_t lcd_spi = 0x302U;
    static constexpr std::uint32_t sd_data = 0x400U;
    static constexpr std::uint32_t sd_control = 0x401U;
    static constexpr std::uint32_t nand_data = 0x402U;
    static constexpr std::uint32_t nand_control = 0x403U;
    static constexpr std::uint32_t cam_data = 0x500U;
    static constexpr std::uint32_t cam_sync = 0x501U;
    static constexpr std::uint32_t console_uart = 0x600U;
    static constexpr std::uint32_t strap_boot = 0x700U;
    static constexpr std::uint32_t strap_pin = 0x701U;
};

using Korvo1CodecGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::I2c::sda, Korvo1::I2c::scl, Korvo1Signal::codec_i2c>>;

using Korvo1AudioGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Audio::mclk, Korvo1::Audio::sclk, Korvo1Signal::audio_clock>,
    PinRoute<Korvo1::Audio::sclk, Korvo1::Audio::lrclk, Korvo1Signal::audio_clock>,
    PinRoute<Korvo1::Audio::sclk, Korvo1::Audio::dsin, Korvo1Signal::audio_data>,
    PinRoute<Korvo1::Audio::sclk, Korvo1::Audio::sdout, Korvo1Signal::audio_data>,
    PinRoute<Korvo1::Audio::pa, Korvo1::Audio::sdout, Korvo1Signal::audio_pa>>;

using Korvo1LcdGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::db0, Korvo1Signal::lcd_data>,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::db7, Korvo1Signal::lcd_data>,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::db15, Korvo1Signal::lcd_data>,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::hen, Korvo1Signal::lcd_sync>,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::hsync, Korvo1Signal::lcd_sync>,
    PinRoute<Korvo1::Lcd::pclk, Korvo1::Lcd::vsync, Korvo1Signal::lcd_sync>,
    PinRoute<Korvo1::Lcd::sck, Korvo1::Lcd::mosi, Korvo1Signal::lcd_spi>,
    PinRoute<Korvo1::Lcd::sck, Korvo1::Lcd::cs, Korvo1Signal::lcd_spi>>;

using Korvo1SdGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Sd::clk, Korvo1::Sd::cmd, Korvo1Signal::sd_control>,
    PinRoute<Korvo1::Sd::clk, Korvo1::Sd::d0, Korvo1Signal::sd_data>,
    PinRoute<Korvo1::Sd::clk, Korvo1::Sd::d1, Korvo1Signal::sd_data>,
    PinRoute<Korvo1::Sd::clk, Korvo1::Sd::d2, Korvo1Signal::sd_data>,
    PinRoute<Korvo1::Sd::clk, Korvo1::Sd::d3, Korvo1Signal::sd_data>,
    PinRoute<Korvo1::Sd::ctrl, Korvo1::Sd::cmd, Korvo1Signal::sd_control>>;

using Korvo1NandGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::SpiNand::clk, Korvo1::SpiNand::d, Korvo1Signal::nand_data>,
    PinRoute<Korvo1::SpiNand::clk, Korvo1::SpiNand::q, Korvo1Signal::nand_data>,
    PinRoute<Korvo1::SpiNand::clk, Korvo1::SpiNand::cs, Korvo1Signal::nand_control>,
    PinRoute<Korvo1::SpiNand::clk, Korvo1::SpiNand::hold, Korvo1Signal::nand_control>,
    PinRoute<Korvo1::SpiNand::clk, Korvo1::SpiNand::wp, Korvo1Signal::nand_control>>;

using Korvo1CamGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Cam::pclk, Korvo1::Cam::d0, Korvo1Signal::cam_data>,
    PinRoute<Korvo1::Cam::pclk, Korvo1::Cam::d7, Korvo1Signal::cam_data>,
    PinRoute<Korvo1::Cam::xclk, Korvo1::Cam::pclk, Korvo1Signal::cam_sync>,
    PinRoute<Korvo1::Cam::pclk, Korvo1::Cam::vsync, Korvo1Signal::cam_sync>,
    PinRoute<Korvo1::Cam::pclk, Korvo1::Cam::hsync, Korvo1Signal::cam_sync>>;

using Korvo1ConsoleGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Uart0::tx, Korvo1::Uart0::rx, Korvo1Signal::console_uart>>;

using Korvo1StrapGraph = TopologyGraph<
    Korvo1,
    PinRoute<Korvo1::Strap::lcd_db15, Korvo1::Strap::b0, Korvo1Signal::strap_pin>,
    PinRoute<Korvo1::Strap::status_led, Korvo1::Strap::b0, Korvo1Signal::strap_pin>,
    PinRoute<Korvo1::Strap::b0, Korvo1::Strap::b1, Korvo1Signal::strap_boot>,
    PinRoute<Korvo1::Strap::b0, Korvo1::Strap::b2, Korvo1Signal::strap_boot>,
    PinRoute<Korvo1::Strap::b0, Korvo1::Strap::b3, Korvo1Signal::strap_boot>,
    PinRoute<Korvo1::Strap::b0, Korvo1::Strap::b4, Korvo1Signal::strap_boot>>;

static_assert(Korvo1CodecGraph::valid());
static_assert(Korvo1AudioGraph::valid());
static_assert(Korvo1LcdGraph::valid());
static_assert(Korvo1SdGraph::valid());
static_assert(Korvo1NandGraph::valid());
static_assert(Korvo1CamGraph::valid());
static_assert(Korvo1ConsoleGraph::valid());
static_assert(Korvo1StrapGraph::valid());

}  // namespace arc::board
