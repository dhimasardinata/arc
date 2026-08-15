from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def write(path: Path, text: str = "") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def add_s31_sdk_metadata(idf: Path) -> None:
    write(idf / "export.sh", ":\n")
    for rel in (
        "components/soc/esp32s31",
        "components/hal/esp32s31",
        "components/esp_rom/esp32s31",
        "components/esp_system/ld/esp32s31",
    ):
        (idf / rel).mkdir(parents=True, exist_ok=True)
    write(idf / "tools" / "cmake" / "toolchain-esp32s31.cmake", "set(_CMAKE_TOOLCHAIN_PREFIX riscv32-esp-elf-)\n")
    write(
        idf / "tools" / "idf_py_actions" / "constants.py",
        "SUPPORTED_TARGETS = ['esp32s3']\nPREVIEW_TARGETS = ['esp32s31']\n",
    )


def add_complete_scaffold(root: Path) -> None:
    write(
        root / "tools" / "s31_manifest.py",
        """
S31_TARGET = "esp32s31"
S31_BOARD = "esp32-s31-korvo-1"
S31_BOARD_HEADER = "arc/board/esp32s31_korvo.hpp"
S31_PREVIEW_IDF_PATH = "/path/to/preview-esp-idf"
S31_EXAMPLES = (
    "amp",
    "audio",
    "cam",
    "console",
    "control",
    "io",
    "lcd",
    "ml",
    "ptp",
    "radio",
    "sd",
    "security",
    "usb",
)
""",
    )
    write(
        root / "components" / "arc" / "include" / "arc" / "board" / "esp32s31_korvo.hpp",
        """
struct Korvo1 {
    static constexpr const char* name = "esp32-s31-korvo-1";
    static constexpr const char* revision = "v1.1";
    static constexpr unsigned flash_mb = 16U;
    static constexpr unsigned psram_mb = 16U;
    struct Module {
        static constexpr const char* model = "ESP32-S31-WROOM-3";
        static constexpr unsigned flash_mb = 16U;
        static constexpr unsigned psram_mb = 16U;
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
        static constexpr unsigned pa_count = 2U;
        static constexpr unsigned mic_count = 2U;
        static constexpr unsigned speaker_count = 2U;
        static constexpr unsigned speaker_ohm = 4U;
        static constexpr unsigned speaker_w = 3U;
        static constexpr unsigned pitch_mm = 2U;
        static constexpr unsigned pitch_mil = 80U;
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
        static constexpr unsigned inch_x10 = 43U;
        static constexpr unsigned hres = 800U;
        static constexpr unsigned vres = 480U;
        static constexpr bool rgb = true;
        static constexpr bool rgb565 = true;
        static constexpr bool touch = true;
    };
    struct Sd {
        static constexpr unsigned width = 4U;
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
        static constexpr unsigned remove_count = 6U;
        static constexpr unsigned base_count = 9U;
        static constexpr unsigned v18_count = 7U;
        static constexpr unsigned v33_count = 1U;
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
        static constexpr unsigned ldo_in_mv = 3300U;
        static constexpr unsigned avdd_mv = 2800U;
        static constexpr unsigned dvdd_mv = 1500U;
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
        static constexpr unsigned max_baud = 3'000'000U;
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
        static constexpr int ws2812 = 37;
        static constexpr const char* signal = "WS2812_CTRL";
        static constexpr unsigned count = 1U;
        static constexpr bool rgb = true;
        static constexpr bool addressable = true;
    };
    struct Usb {
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
        static constexpr unsigned downstream_ma = 500U;
        static constexpr unsigned port = 1U;
    };
    struct Power {
        static constexpr bool power_only = true;
        static constexpr bool uart_power = true;
        static constexpr bool switch_5v = true;
        static constexpr bool audio_split = true;
        static constexpr bool buck_3v3 = true;
        static constexpr bool audio_ldo_3v3 = true;
        static constexpr bool power_led_5v = true;
        static constexpr unsigned input_ma = 3'000U;
    };
    struct Setup {
        static constexpr unsigned usb_cables = 2U;
        static constexpr bool usb2 = true;
        static constexpr bool a_to_c = true;
        static constexpr bool data_cable = true;
        static constexpr unsigned speaker_min = 1U;
        static constexpr unsigned speaker_max = 2U;
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
        using CodecI2c = int;
        using AudioBus = int;
        using AudioPa = int;
        using LcdBus = int;
        using SdNandLane = ClaimFor<ClaimKind::gpio_pin, Sd::d0, ClaimKind::sdmmc_slot, Sd::d0, Sd::d1, Sd::d2, Sd::d3, Sd::clk, Sd::cmd>;
        using SdSlot = ClaimSet<int, SdNandLane>;
        using SdCtrl = int;
        using SpiNandLane = ClaimFor<ClaimKind::gpio_pin, Sd::d0, ClaimKind::spi_bus, SpiNand::clk, SpiNand::d, SpiNand::q, SpiNand::cs, SpiNand::hold, SpiNand::wp>;
        using SpiNandBus = ClaimSet<int, SpiNandLane>;
        using CamBus = int;
        using ConsoleUart = int;
        using ButtonAdc = int;
        using StatusLed = int;
        using UsbOtg = int;
    };
    using pins = arc::Pins<0, 1>;
};
static_assert(Topology<Korvo1>);
struct Korvo1Signal {};
using Korvo1CodecGraph = int;
using Korvo1AudioGraph = int;
using Korvo1LcdGraph = int;
using Korvo1SdGraph = int;
using Korvo1NandGraph = int;
using Korvo1CamGraph = int;
using Korvo1ConsoleGraph = int;
using Korvo1StrapGraph = int;
constexpr int strap_marker = Korvo1Signal::strap_boot;
constexpr int strap_pin_marker = Korvo1Signal::strap_pin;
constexpr int nand_marker = Korvo1Signal::nand_control;
""",
    )
    write(
        root / "components" / "arc" / "include" / "arc" / "soc" / "esp32s31.hpp",
        """
struct Esp32S31 {
    static constexpr const char* name = "esp32s31";
    static constexpr bool experimental = true;
    static constexpr bool wifi6 = true;
    static constexpr bool ble = true;
    static constexpr bool bt54 = true;
    static constexpr bool bt_classic = true;
    static constexpr bool ieee802154 = true;
    static constexpr bool ethernet_mac = true;
    static constexpr bool secure_boot = true;
    static constexpr bool flash_encryption = true;
    static constexpr bool tee = true;
    static constexpr bool puf = true;
    static constexpr bool worldguard = true;
    struct Api {
        static constexpr bool amp = false;
        static constexpr bool cam = true;
        static constexpr bool control = true;
    };
};
""",
    )
    write(
        root / "env.sh",
        """
echo "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON"
echo "complete esp32s31 target metadata"
echo "components/soc/esp32s31"
echo "components/hal/esp32s31"
echo "components/esp_rom/esp32s31"
echo "components/esp_system/ld/esp32s31"
echo "tools/cmake/toolchain-esp32s31.cmake"
echo "tools/idf_py_actions/constants.py"
echo "Unsupported ARC_TARGET"
export IDF_TARGET="${arc_target}"
""",
    )
    write(
        root / "env.fish",
        """
echo "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON"
echo "complete esp32s31 target metadata"
echo "components/soc/esp32s31"
echo "components/hal/esp32s31"
echo "components/esp_rom/esp32s31"
echo "components/esp_system/ld/esp32s31"
echo "tools/cmake/toolchain-esp32s31.cmake"
echo "tools/idf_py_actions/constants.py"
echo "Unsupported ARC_TARGET"
set -gx IDF_TARGET "$arc_target"
""",
    )
    write(
        root / "examples" / "esp32s31" / "sdkconfig.defaults",
        """
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="../../../partitions_16mb.csv"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM_TYPE_AUTO=y
# CONFIG_SPIRAM_TYPE_ESPPSRAM64 is not set
""",
    )
    write(
        root / "examples" / "esp32s31" / "README.md",
        """
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio
python3 tools/s31-build.py --list-ports
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run
Flashing or monitoring requires exactly one `--example` and either `--port` or `--auto-port`
Korvo1::Resource
Korvo1*Graph
dp_module_pin
dm_module_pin
not GPIO numbers
""",
    )
    write(
        root / "README.md",
        """
Example S31 configure flow
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example ptp --auto-port --monitor --dry-run
Flashing or monitoring requires either `--port` or `--auto-port`
Use `--auto-port` only when exactly one Korvo serial port is connected
Use `--auto-port --monitor` only for the single-connected-board case
""",
    )
    write(
        root / "docs" / "modules" / "arc-board-esp32s31_korvo.md",
        """
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run
""",
    )
    for name in (
        "amp",
        "audio",
        "cam",
        "console",
        "control",
        "io",
        "lcd",
        "ml",
        "ptp",
        "radio",
        "sd",
        "security",
        "usb",
    ):
        features = {"audio": "core gpio i2c i2s", "cam": "core cam lcd"}.get(name, "core")
        if name == "lcd":
            features = "core lcd"
        if name == "console":
            features = "core uart"
        if name == "io":
            features = "core adc rmt"
        if name == "sd":
            features = "core gpio sd"
        if name == "usb":
            features = "core otg usb_device usb_host"
        if name == "radio":
            features = "core espnow ble_mesh thread"
        if name == "security":
            features = "core secure_boot puf cloak"
        write(
            root / "examples" / "esp32s31" / name / "CMakeLists.txt",
            """
set(ARC_SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/../sdkconfig.defaults")
arc_target(esp32s31)
""",
        )
        write(
            root / "examples" / "esp32s31" / name / "main" / "CMakeLists.txt",
            f"""
include(${{CMAKE_CURRENT_LIST_DIR}}/../../../../cmake/arc-deps.cmake)
arc_requires(main_requires {features})
""",
        )
        write(
            root / "examples" / "esp32s31" / name / "README.md",
            f"""
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example {name} --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example {name}
""",
        )
        driver_contract = ""
        soc_contract = ""
        if name == "audio":
            soc_contract = "static_assert(arc::Soc::i2c && arc::Soc::i2s);"
            driver_contract = """
#define ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT 1
struct FakeAmpEnable {
    static constexpr unsigned mask() { return 128U; }
};
using AmpEnable = FakeAmpEnable;
using CodecBus = int;
using AudioLink = int;
static_assert(AmpEnable::mask() == 128U);
static_assert(sizeof(Board::Resource::AudioBus) > 0U);
static_assert(sizeof(Board::Resource::AudioPa) > 0U);
static_assert(sizeof(Board::Resource::CodecI2c) > 0U);
static_assert(sizeof(CodecBus::Resource) > 0U);
static_assert(sizeof(AudioLink::Resource) > 0U);
static_assert(Board::Audio::pa == 7);
static_assert(Board::AudioCodec::pa_count == 2U);
static_assert(Board::AudioCodec::mic_count == 2U);
static_assert(Board::AudioCodec::speaker_count == 2U);
static_assert(Board::AudioCodec::pitch_mm == 2U);
static_assert(Board::AudioCodec::pitch_mil == 80U);
static_assert(Board::AudioCodec::stereo);
static_assert(Board::AudioCodec::analog_mics);
static_assert(Board::AudioCodec::speech);
static_assert(Board::AudioCodec::near_wake);
static_assert(Board::AudioCodec::far_wake);
static_assert(Board::Setup::speaker_min == 1U);
static_assert(Board::Setup::speaker_max == Board::AudioCodec::speaker_count);
static_assert(Board::Power::audio_split);
"""
        elif name == "cam":
            soc_contract = "static_assert(arc::Soc::dvp && arc::Soc::lcd_rgb);"
            driver_contract = """
#define ARC_S31_KORVO_CAM_DRIVER_CONTRACT 1
using CamPath = int;
using LcdPath = int;
static_assert(sizeof(Board::Resource::CamBus) > 0U);
static_assert(sizeof(Board::Resource::LcdBus) > 0U);
static_assert(sizeof(CamPath::Resource) > 0U);
static_assert(sizeof(LcdPath::Resource) > 0U);
static_assert(Board::Lcd::db0 == 8);
static_assert(Board::Lcd::db15 == 36);
static_assert(Board::CamModule::ldo_in_mv == 3300U);
static_assert(Board::CamModule::avdd_mv == 2800U);
static_assert(Board::CamModule::dvdd_mv == 1500U);
static_assert(Board::CamModule::avdd_ldo);
static_assert(Board::CamModule::dvdd_ldo);
static_assert(Board::CamModule::video_stream);
static_assert(Board::CamModule::jpeg_stream);
static_assert(Board::Display::external);
static_assert(Board::Display::panel_driver);
static_assert(Board::Display::touch_driver);
static_assert(Board::Display::hres == 800U);
static_assert(Board::Display::vres == 480U);
static_assert(Board::Display::rgb565);
using CameraLcdGraph = Korvo1LcdGraph;
"""
        elif name == "sd":
            soc_contract = "static_assert(arc::Soc::sdmmc);"
            driver_contract = """
#define ARC_S31_KORVO_SD_DRIVER_CONTRACT 1
struct FakeCardCtrl {
    static constexpr unsigned mask() { return 128U; }
};
using CardCtrl = FakeCardCtrl;
using Storage = int;
static_assert(CardCtrl::mask() == 128U);
static_assert(sizeof(Board::Resource::SdSlot) > 0U);
static_assert(sizeof(Storage::Resource) > 0U);
static_assert(sizeof(Board::Resource::SdCtrl) > 0U);
static_assert(Board::Onboard::microsd);
static_assert(Board::Sd::width == 4U);
static_assert(Board::Sd::sdio3);
static_assert(Board::Sd::audio_store);
static_assert(Board::Sd::playback);
static_assert(!Board::Onboard::spi_nand);
static_assert(Board::SpiNand::shares_sd);
static_assert(Board::SpiNand::requires_rework);
static_assert(Board::SpiNand::supports_1v8);
static_assert(Board::SpiNand::supports_3v3);
static_assert(Board::SpiNand::remove_count == 6U);
static_assert(Board::SpiNand::base_count == 9U);
static_assert(Board::SpiNand::v18_count == 7U);
static_assert(Board::SpiNand::v33_count == 1U);
static_assert(Board::Sd::d0 == Board::SpiNand::clk);
static_assert(Board::Sd::cmd == Board::SpiNand::wp);
static_assert(Board::Sd::ctrl == 39);
static_assert(Board::Sd::ctrl == Board::Strap::b1);
using NandAliasGraph = Korvo1NandGraph;
using SdStrapGraph = Korvo1StrapGraph;
void nand_shared();
void sd_strap_edges();
"""
        elif name == "io":
            soc_contract = "static_assert(arc::Soc::adc && arc::Soc::rmt);"
            driver_contract = """
#define ARC_S31_KORVO_IO_DRIVER_CONTRACT 1
using ButtonPad = int;
using ButtonBus = int;
using Button = int;
static_assert(sizeof(Button::Resource) > 0U);
static_assert(Board::Button::signal);
static_assert(Board::Button::count == 4);
static_assert(Board::Button::play == 0);
static_assert(Board::Button::vol_down == 2);
static_assert(Board::Button::ui_control);
static_assert(Board::Button::audio_test);
#define ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT 1
template <int, int, int, int, bool> struct FakeBurst {};
namespace arc { template <int, int, int, int, bool> using Burst = FakeBurst<0, 0, 0, 0, false>; }
using StatusLed = arc::Burst<Board::Led::ws2812, 10000000, 48, 1, false>;
static_assert(sizeof(StatusLed::Resource) > 0U);
static_assert(Board::Led::signal);
static_assert(Board::Led::count == 1U);
static_assert(Board::Led::rgb);
static_assert(Board::Led::addressable);
static_assert(Board::Led::ws2812 == Board::Strap::status_led);
void status_frame();
using ButtonAdc = Board::Resource::ButtonAdc;
using StatusLedResource = Board::Resource::StatusLed;
"""
        elif name == "lcd":
            soc_contract = "static_assert(arc::Soc::lcd_rgb);"
            driver_contract = """
#define ARC_S31_KORVO_LCD_DRIVER_CONTRACT 1
namespace arc { template <int...> struct RgbLines {}; template <typename, int, int, int, int, int, unsigned H, unsigned V> struct Rgb { using Resource = int; static constexpr unsigned width() { return 16U; } static constexpr unsigned h() { return H; } static constexpr unsigned v() { return V; } }; }
using LcdPanel = arc::Rgb<
    arc::RgbLines<
        Board::Lcd::db0,
        Board::Lcd::db15>,
    Board::Lcd::hsync,
    Board::Lcd::vsync,
    Board::Lcd::hen,
    Board::Lcd::pclk,
    -1,
    Board::Display::hres,
    Board::Display::vres>;
static_assert(sizeof(LcdPanel::Resource) > 0U);
static_assert(LcdPanel::width() == 16U);
static_assert(LcdPanel::h() == Board::Display::hres);
static_assert(LcdPanel::v() == Board::Display::vres);
static_assert(sizeof(Board::Resource::LcdBus) > 0U);
static_assert(Board::Display::external);
static_assert(Board::Display::panel_driver);
static_assert(Board::Display::touch_driver);
static_assert(Board::Display::hres == 800U);
static_assert(Board::Display::vres == 480U);
static_assert(Board::Display::rgb565);
static_assert(Board::Lcd::db15 == Board::Strap::lcd_db15);
static_assert(Board::Lcd::cs == 38);
static_assert(Board::Lcd::mosi == 60);
static_assert(Board::Lcd::sck == 61);
using LcdGraph = Korvo1LcdGraph;
using LcdStrapGraph = Korvo1StrapGraph;
static_assert(Board::Lcd::cs == Board::Strap::b0);
static_assert(Board::Lcd::pclk == Board::Strap::b2);
static_assert(Board::Lcd::mosi == Board::Strap::b3);
static_assert(Board::Lcd::sck == Board::Strap::b4);
void lcd_strap_edges();
void lcd_probe_frame();
"""
        elif name == "console":
            soc_contract = "static_assert(arc::Soc::uart);"
            driver_contract = """
#define ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT 1
using Console = int;
static_assert(sizeof(Console::Resource) > 0U);
static_assert(Board::ConsoleBridge::usb_c);
static_assert(Board::ConsoleBridge::powers_board);
static_assert(Board::ConsoleBridge::max_baud == 3'000'000U);
static_assert(Board::Power::uart_power);
static_assert(Board::Download::auto_download);
static_assert(Board::Download::dtr_rts);
static_assert(Board::Download::boot_btn && Board::Download::rst_btn);
static_assert(Board::Setup::usb_cables == 2U);
static_assert(Board::Setup::data_cable);
static_assert(Board::Setup::switch_on);
static_assert(Board::Setup::red_led);
using ConsoleUart = Board::Resource::ConsoleUart;
using ConsoleGraph = Korvo1ConsoleGraph;
"""
        elif name == "usb":
            soc_contract = "static_assert(arc::Soc::usb_otg);"
            driver_contract = """
#include <type_traits>
#define ARC_S31_KORVO_USB_PHY_CONTRACT 1
struct FakeUsbPhy {
    using Resource = Board::Resource::UsbOtg;
};
using UsbPhy = FakeUsbPhy;
static_assert(std::is_same_v<Board::Resource::UsbOtg, UsbPhy::Resource>);
static_assert(sizeof(Board::Resource::UsbOtg) > 0U);
static_assert(sizeof(UsbPhy::Resource) > 0U);
static_assert(Board::Usb::dp_module_pin == 40);
static_assert(Board::Usb::dm_module_pin == 41);
static_assert(Board::Usb::dp_gpio == -1);
static_assert(Board::Usb::dm_gpio == -1);
static_assert(!Board::Usb::module_pins_are_gpio);
static_assert(!Board::pins::has<Board::Usb::dp_gpio>());
static_assert(!Board::pins::has<Board::Usb::dm_gpio>());
inline constexpr const char* usb_log_fields = "usb_dp_module_pin usb_dm_module_pin";
static_assert(Board::UsbHost::type_a);
static_assert(Board::UsbHost::high_speed);
static_assert(Board::UsbHost::current_limited);
static_assert(Board::UsbHost::switch_model);
static_assert(Board::UsbHost::downstream_ma == 500U);
static_assert(Board::Power::input_ma == 3'000U);
static_assert(Board::Power::buck_3v3);
static_assert(Board::Power::power_led_5v);
namespace arc::usb {
struct DeviceDescriptor {};
template <int, int, int> struct Cdc {};
struct HostConfig {};
}
using UsbDeviceDescriptor = arc::usb::DeviceDescriptor;
using UsbCdc = arc::usb::Cdc<0x83, 0x01, 0x82>;
using UsbHostConfig = arc::usb::HostConfig;
static_assert(Board::Onboard::usb_otg);
"""
        elif name == "radio":
            soc_contract = "static_assert(arc::Soc::wifi && arc::Soc::ble && arc::Soc::ble_mesh);"
            driver_contract = """
#define ARC_S31_KORVO_ESPNOW_CONTRACT 1
namespace arc {
namespace ble {
struct Mesh {
    template <typename Policy>
    static int provision(int) { return Policy::mesh_provision(); }
    template <typename Policy>
    static int publish(int, int, int) { return Policy::mesh_publish(); }
};
}
namespace net {
struct Thread {
    template <typename Policy>
    static int attach(int) { return Policy::thread_attach(); }
    template <typename Policy>
    static int send(int, int) { return Policy::thread_send(); }
};
}
}
struct RadioPolicy {
    static int mesh_provision() { return 0; }
    static int mesh_publish() { return 0; }
    static int thread_attach() { return 0; }
    static int thread_send() { return 0; }
};
static_assert(Board::Wireless::wifi6);
static_assert(Board::Wireless::ble);
static_assert(Board::Wireless::bt54);
static_assert(Board::Wireless::bt_classic);
static_assert(Board::Wireless::ieee802154);
static_assert(Board::Wireless::zigbee3);
static_assert(Board::Wireless::thread14);
static_assert(Board::Wireless::pcb_antenna);
static_assert(Board::Wireless::wifi6 == arc::soc::Target::wifi6);
static_assert(Board::Wireless::ble == arc::soc::Target::ble);
static_assert(Board::Wireless::bt54 == arc::soc::Target::bt54);
static_assert(Board::Wireless::bt_classic == arc::soc::Target::bt_classic);
static_assert(Board::Wireless::ieee802154 == arc::soc::Target::ieee802154);
void radio_contract()
{
    arc::ble::Mesh::provision<RadioPolicy>(0);
    arc::ble::Mesh::publish<RadioPolicy>(0, 0, 0);
    arc::net::Thread::attach<RadioPolicy>(0);
    arc::net::Thread::send<RadioPolicy>(0, 0);
}
"""
        elif name == "ml":
            soc_contract = "static_assert(arc::Soc::simd);"
            driver_contract = """
static_assert(arc::soc::Target::simd);
void ml_probe() { arc::ml::saturate_s8(130); }
"""
        elif name == "security":
            driver_contract = """
namespace arc {
namespace secure {
struct SecureBoot {
    template <typename Policy>
    static int state() { return Policy::boot_state(); }
    template <typename Policy>
    static int digest() { return Policy::boot_digest(); }
    template <typename Policy>
    static int revoke(int) { return Policy::boot_revoke(); }
};
}
template <typename Policy>
struct WorldGuard {
    static int apply(int) { return Policy::world_apply(); }
};
namespace crypto {
struct Puf {
    static int von_neumann(int, int) { return 0; }
    template <typename Policy>
    static int derive_with(int) { return Policy::puf_hash(); }
};
struct Cloak {
    template <typename Policy>
    static int scramble(int, int) { return Policy::cloak(); }
};
}
}
struct SecurityPolicy {
    static int boot_state() { return 0; }
    static int boot_digest() { return 0; }
    static int boot_revoke() { return 0; }
    static int world_apply() { return 0; }
    static int puf_hash() { return 0; }
    static int cloak() { return 0; }
};
static_assert(arc::soc::Target::secure_boot);
static_assert(arc::soc::Target::flash_encryption);
static_assert(arc::soc::Target::tee);
static_assert(arc::soc::Target::puf);
static_assert(arc::soc::Target::worldguard);
static_assert(arc::soc::has<arc::soc::Cap::tee>);
static_assert(arc::soc::has<arc::soc::Cap::world>);
static_assert(Board::Module::flash_mb == 16U);
static_assert(Board::Module::psram_mb == 16U);
void security_contract()
{
    arc::secure::SecureBoot::state<SecurityPolicy>();
    arc::secure::SecureBoot::digest<SecurityPolicy>();
    arc::secure::SecureBoot::revoke<SecurityPolicy>(0);
    arc::WorldGuard<SecurityPolicy>::apply(0);
    arc::crypto::Puf::von_neumann(0, 0);
    arc::crypto::Puf::derive_with<SecurityPolicy>(0);
    arc::crypto::Cloak::scramble<SecurityPolicy>(0, 0);
}
"""
        ptp_contract = ""
        if name == "ptp":
            ptp_contract = """
static_assert(!Board::Onboard::eth_phy);
void external_phy();
"""
        write(
            root / "examples" / "esp32s31" / name / "main" / "app_main.cpp",
            f"""
#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
using Board = arc::board::Korvo1;
static_assert(arc::Topology<Board>);
{driver_contract}
{ptp_contract}
static_assert(arc::soc::s31);
{soc_contract}
void log() {{ puts(Board::name); puts("arc-s31-{name}"); }}
""",
        )
    write(
        root / "tests" / "host" / "esp32s31_compile.cpp",
        """
#define ARC_TARGET_ESP32S31 1
#include "arc/touch.hpp"
using Board = arc::board::Korvo1;
static_assert(arc::soc::s31);
static_assert(arc::Soc::simd);
static_assert(arc::Soc::adc);
static_assert(arc::Soc::rmt);
static_assert(arc::Soc::sdmmc);
static_assert(arc::Soc::usb_otg);
static_assert(!arc::Soc::touch);
static_assert(arc::Soc::wifi);
static_assert(arc::Soc::ble);
static_assert(arc::Soc::ble_mesh);
static_assert(arc::Soc::touch_max == 0U);
static_assert(!arc::soc::has<arc::soc::Cap::amp>);
""",
    )
    write(
        root / "tests" / "host" / "stubs" / "soc" / "soc_caps.h",
        """
#define SOC_HOST_TOUCH 0
#define SOC_HOST_TOUCH_MAX_CHAN_ID 0
#define SOC_TOUCH_SENSOR_SUPPORTED SOC_HOST_TOUCH
#define SOC_TOUCH_MAX_CHAN_ID SOC_HOST_TOUCH_MAX_CHAN_ID
""",
    )
    write(
        root / "components" / "arc" / "include" / "arc" / "fence.hpp",
        """
#if defined(__riscv)
__asm__ __volatile__("fence rw, rw" ::: "memory");
#endif
""",
    )
    write(
        root / "tests" / "host" / "CMakeLists.txt",
        """
set(ARC_S31_EXAMPLES amp audio cam console control io lcd ml ptp radio sd security usb)
add_library(target OBJECT examples/esp32s31/${example}/main/app_main.cpp)
target_compile_definitions(target PRIVATE ARC_TARGET_ESP32S31=1)
add_custom_target(arc-host-s31-examples)
""",
    )
    write(
        root / "cmake" / "arc-idf.cmake",
        """
option(ARC_EXPERIMENTAL_ESP32S31 "gate" OFF)
message(FATAL_ERROR "complete esp32s31 target metadata")
set(_arc_s31_target "$ENV{IDF_PATH}/components/soc/esp32s31")
set(_arc_s31_hal "$ENV{IDF_PATH}/components/hal/esp32s31")
set(_arc_s31_rom "$ENV{IDF_PATH}/components/esp_rom/esp32s31")
set(_arc_s31_ld "$ENV{IDF_PATH}/components/esp_system/ld/esp32s31")
set(_arc_s31_toolchain "$ENV{IDF_PATH}/tools/cmake/toolchain-esp32s31.cmake")
set(_arc_s31_registry "$ENV{IDF_PATH}/tools/idf_py_actions/constants.py")
""",
    )
    write(
        root / "tools" / "arc_idf_test.py",
        """
def test_rejects_esp32s31_without_experimental_gate(): pass
def test_rejects_esp32s31_when_idf_lacks_target_metadata(): pass
def test_accepts_esp32s31_env_gate_with_target_metadata(): pass
""",
    )
    write(
        root / "tools" / "env_loader_test.py",
        """
def test_bash_rejects_s31_without_gate_before_loading_idf(): pass
def test_bash_rejects_s31_with_gate_when_idf_lacks_target(): pass
def test_bash_accepts_s31_with_gate_and_target_metadata(): pass
def test_fish_rejects_s31_with_gate_when_idf_lacks_target(): pass
""",
    )
    write(
        root / "tools" / "compile-fail-check.py",
        """
cases = (
    "s31_bare_core_rejects_unwired_true_amp",
    "s31_touch_bus_rejects_s3_capacitive_touch",
    "s31_touch_rejects_s3_capacitive_touch",
)
messages = (
    "arc::BareCore true AMP is not wired for ESP32-S31",
    "arc::TouchBus is ESP32-S3 capacitive touch only",
    "arc::Touch is ESP32-S3 capacitive touch only",
)
""",
    )
    write(
        root / "tools" / "arc_projects.py",
        """
if rel.startswith("examples/esp32s31/"):
    return ArcProject(path.resolve(), rel, "example", "esp32s31", True)
S31_PREVIEW_IDF_PATH = "/path/to/preview-esp-idf"
path = os.environ.get("S31_PREVIEW_IDF_PATH") or os.environ.get("ARC_IDF_PATH") or S31_PREVIEW_IDF_PATH
def preflight_command(project): pass
preflight = "tools/s31-readiness.py --idf-path"
build = "tools/s31-build.py --idf-path"
example = "--example"
""",
    )
    write(
        root / "tools" / "s31_build.py",
        """
from s31_manifest import S31_EXAMPLES
def run():
    command = "s31-readiness.py"
    env = "ARC_IDF_PATH ARC_TARGET ARC_EXPERIMENTAL_ESP32S31 IDF_TARGET"
    ports = "serial_port_key serial_ports serial/by-id"
    resolver = "resolve_port"
    build = 'parts = ["idf.py", "-C", project]'
    flash = 'actions.append("flash")'
    monitor = 'actions.append("monitor")'
    project = "examples/esp32s31"
    args = "--dry-run --example --port --auto-port --list-ports --flash --monitor"
    error = "requires exactly one --example"
    explicit = "ESP32-S31 flash/monitor requires --port or --auto-port"
    auto = "--auto-port is only used with --flash or --monitor"
    conflict = "--port and --auto-port are mutually exclusive"
""",
    )
    write(
        root / "tools" / "s31-build.py",
        """
from s31_build import main
""",
    )
    write(
        root / "tools" / "s31_build_test.py",
        """
def test_serial_ports_prefers_stable_by_id_then_usb_devices(): pass
def test_serial_ports_deduplicates_by_id_aliases(): pass
def test_auto_port_uses_by_id_alias_when_one_physical_device_exists(): pass
def test_cli_auto_port_rejects_missing_candidates(): pass
def test_cli_auto_port_rejects_multiple_candidates(): pass
def test_cli_flash_requires_explicit_or_auto_port(): pass
def test_cli_monitor_requires_explicit_or_auto_port(): pass
def test_cli_auto_port_requires_flash_or_monitor(): pass
def test_cli_port_and_auto_port_are_mutually_exclusive(): pass
""",
    )
    write(
        root / "tools" / "ci-build-plan.py",
        """
experimental = [project for project in found if project.experimental]
project = project_for(path, experimental)
include_experimental = False
flag = "--include-experimental"
help="Accepted for workflow readability; experimental projects are skipped unless --include-experimental is set."
""",
    )
    write(
        root / ".github" / "workflows" / "build.yml",
        """
on:
  workflow_dispatch:
    inputs:
      include_experimental:
        type: boolean
      idf_ref:
        type: string
env:
  ARC_IDF_EFFECTIVE_REF: ${{ github.event.inputs.idf_ref || 'default' }}
  ARC_INCLUDE_EXPERIMENTAL: ${{ github.event.inputs.include_experimental || 'false' }}
  ARC_IDF_TARGET_SET: esp32s3-esp32s31
  ARC_IDF_INSTALL_TARGETS: esp32s3 esp32s31
run: |
  plan_args=(--buildable)
  plan_args+=(--include-experimental)
  ./tools/ci-build-plan.py "${plan_args[@]}"
  git -C "$HOME/esp-idf" fetch --depth 1 origin "$ARC_IDF_EFFECTIVE_REF"
  git -C "$HOME/esp-idf" checkout --force FETCH_HEAD
  target_set_marker="$HOME/.espressif/arc-idf-target-set-${ARC_IDF_TARGET_SET}"
  ./tools/s31-readiness.py --idf-path "$HOME/esp-idf" --require-sdk
  "$HOME/esp-idf/install.sh" "${idf_targets[@]}"
  export ARC_TARGET=esp32s31
  export ARC_EXPERIMENTAL_ESP32S31=ON
""",
    )


class S31ReadinessTest(unittest.TestCase):
    def run_tool(
        self, root: Path, *args: str, extra_env: dict[str, str] | None = None
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.pop("ARC_IDF_PATH", None)
        env.pop("IDF_PATH", None)
        if extra_env is not None:
            env.update(extra_env)
        return subprocess.run(
            ["python3", "tools/s31-readiness.py", "--root", str(root), *args],
            cwd=ROOT,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

    def test_complete_scaffold_reports_blocked_when_sdk_target_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)

            result = self.run_tool(root, "--format", "report")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("- status: blocked", result.stdout)
        self.assertIn("ESP-IDF target metadata missing", result.stdout)

    def test_require_sdk_fails_when_sdk_target_missing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)

            result = self.run_tool(root, "--require-sdk")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ESP-IDF target metadata missing", result.stderr)

    def test_reports_ready_when_sdk_target_exists(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            add_s31_sdk_metadata(root / "esp-idf")

            result = self.run_tool(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc ESP32-S31 readiness: ready", result.stdout)

    def test_reports_ready_with_explicit_idf_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            idf_path = root / "preview-idf"
            add_s31_sdk_metadata(idf_path)

            result = self.run_tool(root, "--idf-path", str(idf_path), "--require-sdk", "--format", "report")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("- status: ready", result.stdout)
        self.assertIn("- sdk source: --idf-path", result.stdout)

    def test_explicit_idf_path_requires_export_script(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            idf_path = root / "preview-idf"
            add_s31_sdk_metadata(idf_path)
            (idf_path / "export.sh").unlink()

            result = self.run_tool(root, "--idf-path", str(idf_path), "--require-sdk")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("preview-idf/export.sh", result.stderr)

    def test_explicit_idf_path_does_not_fall_back_to_repo_idf(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            add_s31_sdk_metadata(root / "esp-idf")
            idf_path = root / "preview-idf"

            result = self.run_tool(root, "--idf-path", str(idf_path), "--require-sdk")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("preview-idf/components/soc/esp32s31", result.stderr)

    def test_env_probe_skips_stale_arc_idf_path_without_export(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            stale = root / "stale-idf"
            selected = root / "global-idf"
            stale.mkdir()
            add_s31_sdk_metadata(selected)
            (selected / "export.sh").write_text(":", encoding="utf-8")

            result = self.run_tool(
                root,
                "--require-sdk",
                "--format",
                "report",
                extra_env={"ARC_IDF_PATH": str(stale), "IDF_PATH": str(selected)},
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("- sdk source: IDF_PATH", result.stdout)

    def test_env_probe_keeps_arc_idf_path_precedence_when_export_exists(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            selected = root / "arc-idf"
            fallback = root / "global-idf"
            selected.mkdir()
            (selected / "export.sh").write_text(":", encoding="utf-8")
            add_s31_sdk_metadata(fallback)
            (fallback / "export.sh").write_text(":", encoding="utf-8")

            result = self.run_tool(
                root,
                "--require-sdk",
                extra_env={"ARC_IDF_PATH": str(selected), "IDF_PATH": str(fallback)},
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("arc-idf/components/soc/esp32s31", result.stderr)

    def test_missing_korvo_header_is_problem(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            (root / "components" / "arc" / "include" / "arc" / "board" / "esp32s31_korvo.hpp").unlink()

            result = self.run_tool(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Korvo board header missing", result.stderr)

    def test_wrong_korvo_pin_is_problem(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            board = root / "components" / "arc" / "include" / "arc" / "board" / "esp32s31_korvo.hpp"
            text = board.read_text(encoding="utf-8")
            board.write_text(
                text.replace("static constexpr int sdout = 6;", "static constexpr int sdout = 16;"), encoding="utf-8"
            )

            result = self.run_tool(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Audio::sdout expected GPIO6, got GPIO16", result.stderr)

    def test_missing_s31_main_cmake_deps_include_is_problem(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            main_cmake = root / "examples" / "esp32s31" / "audio" / "main" / "CMakeLists.txt"
            main_cmake.write_text("arc_requires(main_requires core gpio i2c i2s)\n", encoding="utf-8")

            result = self.run_tool(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ESP32-S31 audio main CMake shared deps", result.stderr)

    def test_host_s31_example_manifest_drift_is_problem(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_complete_scaffold(root)
            host_cmake = root / "tests" / "host" / "CMakeLists.txt"
            text = host_cmake.read_text(encoding="utf-8")
            host_cmake.write_text(text.replace(" radio", ""), encoding="utf-8")

            result = self.run_tool(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ESP32-S31 example manifest mismatch in tests/host/CMakeLists.txt", result.stderr)


if __name__ == "__main__":
    unittest.main()
