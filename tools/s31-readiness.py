#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from typing import Any

from s31_manifest import S31_BOARD, S31_BOARD_HEADER, S31_EXAMPLES, S31_TARGET

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_EXAMPLES = S31_EXAMPLES
OUTPUT_FORMATS = ("text", "report", "json")
KORVO1_PIN_SOURCE = "ESP32-S31-Korvo-1 V1.1 User Guide pin assignment table"
S31_SDK_REQUIRED_EXPORT = ("shell export", "export.sh")
S31_SDK_REQUIRED_METADATA: tuple[tuple[str, str], ...] = (
    ("soc target", "components/soc/esp32s31"),
    ("HAL target", "components/hal/esp32s31"),
    ("ROM target", "components/esp_rom/esp32s31"),
    ("system linker scripts", "components/esp_system/ld/esp32s31"),
    ("CMake toolchain", "tools/cmake/toolchain-esp32s31.cmake"),
)
S31_SDK_REGISTRY = "tools/idf_py_actions/constants.py"
S31_SDK_GATE_NEEDLES = tuple(rel_path for _, rel_path in S31_SDK_REQUIRED_METADATA) + (S31_SDK_REGISTRY,)
KORVO1_EXPECTED_PINS: dict[str, dict[str, int]] = {
    "Audio": {
        "mclk": 2,
        "sclk": 3,
        "lrclk": 4,
        "dsin": 5,
        "sdout": 6,
        "pa": 7,
    },
    "I2c": {
        "sda": 0,
        "scl": 1,
    },
    "Lcd": {
        "db0": 8,
        "db1": 9,
        "db2": 10,
        "db3": 11,
        "db4": 12,
        "db5": 13,
        "db6": 14,
        "db7": 15,
        "db8": 16,
        "db9": 17,
        "db10": 18,
        "db11": 19,
        "db12": 33,
        "db13": 34,
        "db14": 35,
        "db15": 36,
        "cs": 38,
        "pclk": 40,
        "hen": 43,
        "hsync": 44,
        "vsync": 45,
        "mosi": 60,
        "sck": 61,
    },
    "Sd": {
        "d0": 20,
        "d1": 21,
        "d2": 22,
        "d3": 23,
        "clk": 24,
        "cmd": 25,
        "ctrl": 39,
    },
    "SpiNand": {
        "clk": 20,
        "d": 21,
        "q": 22,
        "cs": 23,
        "hold": 24,
        "wp": 25,
    },
    "Cam": {
        "d0": 46,
        "d1": 47,
        "d2": 48,
        "d3": 49,
        "d4": 50,
        "d5": 51,
        "d6": 52,
        "d7": 53,
        "pclk": 54,
        "xclk": 55,
        "vsync": 56,
        "hsync": 57,
    },
    "Uart0": {
        "tx": 58,
        "rx": 59,
    },
    "Button": {
        "adc": 42,
        "count": 4,
        "play": 0,
        "set": 1,
        "vol_down": 2,
        "vol_up": 3,
    },
    "Led": {
        "ws2812": 37,
    },
    "Strap": {
        "lcd_db15": 36,
        "status_led": 37,
        "b0": 38,
        "b1": 39,
        "b2": 40,
        "b3": 60,
        "b4": 61,
    },
}
INT_FIELD_RE = re.compile(r"static\s+constexpr\s+int\s+(?P<name>\w+)\s*=\s*(?P<value>-?\d+)\s*;")
CMAKE_S31_EXAMPLES_RE = re.compile(r"set\(\s*ARC_S31_EXAMPLES\s+(?P<examples>[^)]*?)\)", re.DOTALL)


def read(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except OSError:
        return ""


def check_contains(path: Path, needles: tuple[str, ...], problems: list[str], label: str) -> None:
    text = read(path)
    if not path.is_file():
        problems.append(f"{label} missing: {path.as_posix()}")
        return
    for needle in needles:
        if needle not in text:
            problems.append(f"{label} missing {needle!r}: {path.as_posix()}")


def struct_body(text: str, name: str) -> str | None:
    match = re.search(rf"\bstruct\s+{re.escape(name)}\s*\{{(?P<body>.*?)\n\s*\}};", text, flags=re.DOTALL)
    return None if match is None else match.group("body")


def int_fields(body: str) -> dict[str, int]:
    return {match.group("name"): int(match.group("value")) for match in INT_FIELD_RE.finditer(body)}


def cmake_s31_examples(path: Path) -> list[str] | None:
    text = read(path)
    match = CMAKE_S31_EXAMPLES_RE.search(text)
    if match is None:
        return None
    return match.group("examples").split()


def check_example_list(label: str, found: list[str] | None, problems: list[str]) -> None:
    expected = list(EXPECTED_EXAMPLES)
    if found is None:
        problems.append(f"ESP32-S31 example manifest missing in {label}")
        return
    if found != expected:
        problems.append(
            f"ESP32-S31 example manifest mismatch in {label}: expected {', '.join(expected)}, got {', '.join(found)}"
        )


def check_korvo_pin_table(path: Path, problems: list[str]) -> None:
    text = read(path)
    if not path.is_file():
        problems.append(f"Korvo V1.1 pin table missing: {path.as_posix()}")
        return

    for section, expected in KORVO1_EXPECTED_PINS.items():
        body = struct_body(text, section)
        if body is None:
            problems.append(f"{KORVO1_PIN_SOURCE}: struct {section} missing in {path.as_posix()}")
            continue
        found = int_fields(body)
        for pin_name, expected_gpio in expected.items():
            actual = found.get(pin_name)
            if actual != expected_gpio:
                got = "missing" if actual is None else f"GPIO{actual}"
                problems.append(f"{KORVO1_PIN_SOURCE}: {section}::{pin_name} expected GPIO{expected_gpio}, got {got}")
        extra = sorted(set(found).difference(expected))
        if extra:
            problems.append(f"{KORVO1_PIN_SOURCE}: struct {section} has unexpected pins: {', '.join(extra)}")


def rel(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def has_idf_export(path: Path) -> bool:
    return (path / "export.sh").is_file() or (path / "export.fish").is_file()


def sdk_target_metadata(root: Path, idf: Path) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []
    missing: list[str] = []
    export_label, export_rel_path = S31_SDK_REQUIRED_EXPORT
    export_path = idf / export_rel_path
    export_ok = export_path.is_file()
    checks.append({"label": export_label, "path": rel(export_path, root), "available": export_ok})
    if not export_ok:
        missing.append(f"{export_label}:{rel(export_path, root)}")

    for label, rel_path in S31_SDK_REQUIRED_METADATA:
        path = idf / rel_path
        ok = path.exists()
        checks.append({"label": label, "path": rel(path, root), "available": ok})
        if not ok:
            missing.append(f"{label}:{rel(path, root)}")

    registry = idf / S31_SDK_REGISTRY
    registry_text = read(registry)
    registered = "'esp32s31'" in registry_text or '"esp32s31"' in registry_text
    checks.append({"label": "IDF target registry", "path": rel(registry, root), "available": registered})
    if not registered:
        missing.append(f"IDF target registry:{rel(registry, root)}")

    return {"available": not missing, "checks": checks, "missing": missing}


def sdk_target_probe(root: Path, idf_path: Path | None = None) -> dict[str, Any]:
    seen: set[str] = set()
    candidates: list[dict[str, Any]] = []

    def add(source: str, path: Path | str | None, *, require_export: bool = False) -> None:
        if path is None or str(path) == "":
            return
        candidate_root = Path(path).expanduser()
        if require_export and not has_idf_export(candidate_root):
            return
        key = candidate_root.resolve().as_posix() if candidate_root.exists() else candidate_root.as_posix()
        if key in seen:
            return
        seen.add(key)
        metadata = sdk_target_metadata(root, candidate_root)
        target = candidate_root / S31_SDK_REQUIRED_METADATA[0][1]
        candidates.append(
            {
                "source": source,
                "root": rel(candidate_root, root),
                "path": rel(target, root),
                "available": metadata["available"],
                "missing": metadata["missing"],
                "checks": metadata["checks"],
            }
        )

    if idf_path is not None:
        add("--idf-path", idf_path)
    else:
        add("ARC_IDF_PATH", os.environ.get("ARC_IDF_PATH"), require_export=True)
        add("IDF_PATH", os.environ.get("IDF_PATH"), require_export=True)
        add("repo", root / "esp-idf")

    selected = candidates[0]
    return {
        "source": selected["source"],
        "path": selected["path"],
        "available": selected["available"],
        "missing": selected["missing"],
        "checks": selected["checks"],
        "checked": candidates,
    }


def readiness(root: Path, require_sdk: bool = False, idf_path: Path | None = None) -> dict[str, Any]:
    problems: list[str] = []
    blockers: list[str] = []

    sdk_target = sdk_target_probe(root, idf_path)
    if not sdk_target["available"]:
        checked = ", ".join(f"{candidate['source']}:{candidate['path']}" for candidate in sdk_target["checked"])
        missing = ", ".join(sdk_target["missing"])
        blockers.append(
            f"ESP-IDF target metadata missing/incomplete: {sdk_target['path']} (checked {checked}; missing {missing})"
        )

    check_contains(
        root / "tools" / "s31_manifest.py",
        (
            "S31_EXAMPLES",
            f'S31_TARGET = "{S31_TARGET}"',
            f'S31_BOARD = "{S31_BOARD}"',
            f'S31_BOARD_HEADER = "{S31_BOARD_HEADER}"',
            "S31_PREVIEW_IDF_PATH",
            *EXPECTED_EXAMPLES,
        ),
        problems,
        "ESP32-S31 manifest",
    )

    board = root / "components" / "arc" / "include" / S31_BOARD_HEADER
    check_contains(
        board,
        (
            "struct Korvo1",
            f'name = "{S31_BOARD}"',
            'revision = "v1.1"',
            "flash_mb = 16U",
            "psram_mb = 16U",
            "struct Module",
            'model = "ESP32-S31-WROOM-3"',
            "pcb_antenna = true",
            "struct Wireless",
            "wifi = true",
            "wifi6 = true",
            "ble = true",
            "bt54 = true",
            "bt_classic = true",
            "ieee802154 = true",
            "zigbee3 = true",
            "thread14 = true",
            "pcb_antenna = Module::pcb_antenna",
            "struct Onboard",
            "audio = true",
            "lcd = true",
            "camera = true",
            "microsd = true",
            "button = true",
            "status_led = true",
            "spi_nand = false",
            "usb_otg = true",
            "eth_phy = false",
            "shared_adc = true",
            "struct AudioCodec",
            'model = "ES8389"',
            'pa_model = "NS4150B"',
            "pa_count = 2U",
            "mic_count = 2U",
            "speaker_count = 2U",
            "speaker_ohm = 4U",
            "speaker_w = 3U",
            "pitch_mm = 2U",
            "pitch_mil = 80U",
            "analog_mics = true",
            "speech = true",
            "near_wake = true",
            "far_wake = true",
            "struct Display",
            "external = true",
            "connector = true",
            'expansion = "ESP32-S3-LCD-EV-Board-SUB3"',
            'panel_driver = "ST7262E43"',
            'touch_driver = "GT1151"',
            "inch_x10 = 43U",
            "hres = 800U",
            "vres = 480U",
            "rgb = true",
            "rgb565 = true",
            "touch = true",
            "width = 4U",
            "sdio3 = true",
            "audio_store = true",
            "playback = true",
            "struct CamModule",
            'model = "OV3660"',
            "ldo_in_mv = 3300U",
            "avdd_mv = 2800U",
            "dvdd_mv = 1500U",
            "avdd_ldo = true",
            "dvdd_ldo = true",
            "video_stream = true",
            "jpeg_stream = true",
            "struct ConsoleBridge",
            "usb_c = true",
            "powers_board = true",
            "flash = true",
            "max_baud = 3'000'000U",
            "struct Download",
            "uart = true",
            "manual = true",
            "auto_download = true",
            "dtr_rts = true",
            "boot_btn = true",
            "rst_btn = true",
            'signal = "ADC BUTTON"',
            "ui_control = true",
            "audio_test = true",
            'signal = "WS2812_CTRL"',
            "count = 1U",
            "rgb = true",
            "addressable = true",
            "struct Usb",
            "dp_module_pin = 40",
            "dm_module_pin = 41",
            "dp_gpio = -1",
            "dm_gpio = -1",
            "module_pins_are_gpio = false",
            "struct UsbHost",
            "type_a = true",
            "high_speed = true",
            "downstream_power = true",
            "current_limited = true",
            'switch_model = "TPS2051C"',
            "downstream_ma = 500U",
            "struct Power",
            "power_only = true",
            "uart_power = true",
            "switch_5v = true",
            "audio_split = true",
            "buck_3v3 = true",
            "audio_ldo_3v3 = true",
            "power_led_5v = true",
            "input_ma = 3'000U",
            "struct Setup",
            "usb_cables = 2U",
            "usb2 = true",
            "a_to_c = true",
            "data_cable = true",
            "speaker_min = 1U",
            "speaker_max = 2U",
            "switch_on = true",
            "red_led = true",
            "microsd_optional = true",
            "struct SpiNand",
            "connected = false",
            "shares_sd = true",
            "requires_rework = true",
            "supports_1v8 = true",
            "supports_3v3 = true",
            'remove = "R7,R65,R66,R67,R68,R69"',
            'base_pop = "R22,R23,R1,R2,R3,R4,C6,R20,U4"',
            'v18_pop = "R134,C66,C80,R100,U1,C82,C67"',
            'v33_pop = "R135"',
            "remove_count = 6U",
            "base_count = 9U",
            "v18_count = 7U",
            "v33_count = 1U",
            "struct Resource",
            "using CodecI2c",
            "using AudioBus",
            "using AudioPa",
            "using LcdBus",
            "using SdNandLane",
            "ClaimKind::sdmmc_slot",
            "using SdSlot = ClaimSet",
            "SdNandLane",
            "using SdCtrl",
            "using SpiNandLane",
            "ClaimKind::spi_bus",
            "using SpiNandBus = ClaimSet",
            "SpiNandLane",
            "using CamBus",
            "using ConsoleUart",
            "using ButtonAdc",
            "using StatusLed",
            "using UsbOtg",
            "struct Korvo1Signal",
            "using Korvo1CodecGraph",
            "using Korvo1AudioGraph",
            "using Korvo1LcdGraph",
            "using Korvo1SdGraph",
            "using Korvo1NandGraph",
            "using Korvo1CamGraph",
            "using Korvo1ConsoleGraph",
            "using Korvo1StrapGraph",
            "using pins = arc::Pins<",
            "static_assert(Topology<Korvo1>)",
            "Korvo1Signal::strap_boot",
            "Korvo1Signal::strap_pin",
            "Korvo1Signal::nand_control",
        ),
        problems,
        "Korvo board header",
    )
    check_korvo_pin_table(board, problems)

    s31_soc = root / "components" / "arc" / "include" / "arc" / "soc" / "esp32s31.hpp"
    check_contains(
        s31_soc,
        (
            'name = "esp32s31"',
            "experimental = true",
            "wifi6 = true",
            "ble = true",
            "bt54 = true",
            "bt_classic = true",
            "ieee802154 = true",
            "ethernet_mac = true",
            "secure_boot = true",
            "flash_encryption = true",
            "tee = true",
            "puf = true",
            "worldguard = true",
            "amp = false",
            "cam = true",
            "control = true",
        ),
        problems,
        "ESP32-S31 SoC facts",
    )

    env_sh = root / "env.sh"
    env_fish = root / "env.fish"
    check_contains(
        env_sh,
        (
            "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON",
            "complete esp32s31 target metadata",
            *S31_SDK_GATE_NEEDLES,
            "Unsupported ARC_TARGET",
            'export IDF_TARGET="${arc_target}"',
        ),
        problems,
        "bash env loader",
    )
    check_contains(
        env_fish,
        (
            "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON",
            "complete esp32s31 target metadata",
            *S31_SDK_GATE_NEEDLES,
            "Unsupported ARC_TARGET",
            'set -gx IDF_TARGET "$arc_target"',
        ),
        problems,
        "fish env loader",
    )

    shared_defaults = root / "examples" / "esp32s31" / "sdkconfig.defaults"
    check_contains(
        shared_defaults,
        (
            'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="../../../partitions_16mb.csv"',
            "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y",
            "CONFIG_SPIRAM_TYPE_AUTO=y",
            "# CONFIG_SPIRAM_TYPE_ESPPSRAM64 is not set",
        ),
        problems,
        "ESP32-S31 shared sdkconfig defaults",
    )

    example_root = root / "examples" / "esp32s31"
    check_contains(
        example_root / "README.md",
        (
            "export ARC_IDF_PATH=/path/to/preview-esp-idf",
            'tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report',
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --dry-run',
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio',
            "tools/s31-build.py --list-ports",
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor --dry-run',
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor',
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run',
            "Flashing or monitoring requires exactly one `--example` and either `--port` or `--auto-port`",
            "Korvo1::Resource",
            "Korvo1*Graph",
            "dp_module_pin",
            "dm_module_pin",
            "not GPIO numbers",
        ),
        problems,
        "ESP32-S31 root README preflight",
    )
    check_contains(
        root / "README.md",
        (
            "Example S31 configure flow",
            'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example ptp --auto-port --monitor --dry-run',
            "Flashing or monitoring requires either `--port` or `--auto-port`",
            "exactly one Korvo serial port is connected",
            "Use `--auto-port --monitor` only for the single-connected-board case",
        ),
        problems,
        "ESP32-S31 main README board workflow",
    )
    check_contains(
        root / "docs" / "modules" / "arc-board-esp32s31_korvo.md",
        ('tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run',),
        problems,
        "ESP32-S31 Korvo module board workflow",
    )
    found_examples = (
        sorted(path.name for path in example_root.iterdir() if path.is_dir() and (path / "main").is_dir())
        if example_root.is_dir()
        else []
    )
    missing_examples = sorted(set(EXPECTED_EXAMPLES).difference(found_examples))
    extra_examples = sorted(set(found_examples).difference(EXPECTED_EXAMPLES))
    for name in missing_examples:
        problems.append(f"ESP32-S31 example missing: examples/esp32s31/{name}")
    for name in extra_examples:
        problems.append(f"unexpected ESP32-S31 example: examples/esp32s31/{name}")

    for name in EXPECTED_EXAMPLES:
        cmake = example_root / name / "CMakeLists.txt"
        check_contains(
            cmake,
            (
                '"${CMAKE_CURRENT_LIST_DIR}/../sdkconfig.defaults"',
                "arc_target(esp32s31)",
            ),
            problems,
            f"ESP32-S31 {name} CMake",
        )
        main_cmake = example_root / name / "main" / "CMakeLists.txt"
        check_contains(
            main_cmake,
            (
                "include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/arc-deps.cmake)",
                "arc_requires(main_requires",
            ),
            problems,
            f"ESP32-S31 {name} main CMake shared deps",
        )
        main = example_root / name / "main" / "app_main.cpp"
        check_contains(
            main,
            (
                f'#include "{S31_BOARD_HEADER}"',
                "static_assert(arc::soc::s31",
                "using Board = arc::board::Korvo1",
                "static_assert(arc::Topology<Board>)",
                "Board::name",
                f"arc-s31-{name}",
            ),
            problems,
            f"ESP32-S31 {name} app",
        )
        check_contains(
            example_root / name / "README.md",
            (
                "export ARC_IDF_PATH=/path/to/preview-esp-idf",
                'tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report',
                f'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example {name} --dry-run',
                f'tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example {name}',
            ),
            problems,
            f"ESP32-S31 {name} README preflight",
        )

    check_contains(
        example_root / "audio" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core gpio i2c i2s)",),
        problems,
        "ESP32-S31 audio main CMake",
    )
    check_contains(
        example_root / "audio" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_AUDIO_DRIVER_CONTRACT",
            "using AmpEnable",
            "using CodecBus",
            "using AudioLink",
            "arc::Soc::i2c && arc::Soc::i2s",
            "AmpEnable::mask()",
            "Board::Resource::AudioBus",
            "Board::Resource::AudioPa",
            "Board::Resource::CodecI2c",
            "CodecBus::Resource",
            "AudioLink::Resource",
            "Board::Audio::pa == 7",
            "Board::AudioCodec::pa_count == 2U",
            "Board::AudioCodec::mic_count == 2U",
            "Board::AudioCodec::speaker_count == 2U",
            "Board::AudioCodec::pitch_mm == 2U",
            "Board::AudioCodec::stereo",
            "Board::AudioCodec::analog_mics",
            "Board::AudioCodec::speech",
            "Board::AudioCodec::near_wake",
            "Board::AudioCodec::far_wake",
            "Board::Setup::speaker_min == 1U",
            "Board::Setup::speaker_max == Board::AudioCodec::speaker_count",
            "Board::Power::audio_split",
        ),
        problems,
        "ESP32-S31 audio driver contract",
    )
    check_contains(
        example_root / "cam" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core cam lcd)",),
        problems,
        "ESP32-S31 cam main CMake",
    )
    check_contains(
        example_root / "cam" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_CAM_DRIVER_CONTRACT",
            "using CamPath",
            "using LcdPath",
            "arc::Soc::dvp && arc::Soc::lcd_rgb",
            "Board::Resource::CamBus",
            "Board::Resource::LcdBus",
            "CamPath::Resource",
            "LcdPath::Resource",
            "Board::Lcd::db0",
            "Board::Lcd::db15",
            "Board::Display::panel_driver",
            "Board::Display::touch_driver",
            "Board::Display::hres == 800U",
            "Board::Display::vres == 480U",
            "Board::Display::rgb565",
            "Board::CamModule::ldo_in_mv == 3300U",
            "Board::CamModule::avdd_mv == 2800U",
            "Board::CamModule::dvdd_mv == 1500U",
            "Board::CamModule::avdd_ldo",
            "Board::CamModule::dvdd_ldo",
            "Board::CamModule::video_stream",
            "Board::CamModule::jpeg_stream",
            "Board::Display::external",
            "Korvo1LcdGraph",
        ),
        problems,
        "ESP32-S31 cam driver contract",
    )
    check_contains(
        example_root / "console" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core uart)",),
        problems,
        "ESP32-S31 console main CMake",
    )
    check_contains(
        example_root / "console" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_CONSOLE_DRIVER_CONTRACT",
            "using Console",
            "arc::Soc::uart",
            "Console::Resource",
            "Korvo1ConsoleGraph",
            "Board::Resource::ConsoleUart",
            "Board::ConsoleBridge::usb_c",
            "Board::ConsoleBridge::powers_board",
            "Board::ConsoleBridge::max_baud == 3'000'000U",
            "Board::Power::uart_power",
            "Board::Download::auto_download",
            "Board::Download::dtr_rts",
            "Board::Download::boot_btn && Board::Download::rst_btn",
            "Board::Setup::usb_cables == 2U",
            "Board::Setup::data_cable",
            "Board::Setup::switch_on",
            "Board::Setup::red_led",
        ),
        problems,
        "ESP32-S31 console driver contract",
    )
    check_contains(
        example_root / "io" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core adc rmt)",),
        problems,
        "ESP32-S31 IO main CMake",
    )
    check_contains(
        example_root / "io" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_IO_DRIVER_CONTRACT",
            "using ButtonPad",
            "using ButtonBus",
            "using Button",
            "arc::Soc::adc && arc::Soc::rmt",
            "Button::Resource",
            "ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT",
            "using StatusLed",
            "arc::Burst<Board::Led::ws2812",
            "StatusLed::Resource",
            "status_frame",
            "Board::Resource::ButtonAdc",
            "Board::Resource::StatusLed",
            "Board::Button::count == 4",
            "Board::Button::play == 0",
            "Board::Button::vol_down == 2",
            "Board::Button::signal",
            "Board::Button::ui_control",
            "Board::Button::audio_test",
            "Board::Led::signal",
            "Board::Led::count == 1U",
            "Board::Led::rgb",
            "Board::Led::addressable",
            "Board::Led::ws2812 == Board::Strap::status_led",
        ),
        problems,
        "ESP32-S31 IO driver contract",
    )
    check_contains(
        example_root / "lcd" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core lcd)",),
        problems,
        "ESP32-S31 LCD main CMake",
    )
    check_contains(
        example_root / "lcd" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_LCD_DRIVER_CONTRACT",
            "using LcdPanel",
            "arc::Soc::lcd_rgb",
            "arc::Rgb<",
            "arc::RgbLines<",
            "LcdPanel::Resource",
            "LcdPanel::width() == 16U",
            "LcdPanel::h() == Board::Display::hres",
            "LcdPanel::v() == Board::Display::vres",
            "Board::Resource::LcdBus",
            "Board::Display::external",
            "Board::Display::panel_driver",
            "Board::Display::touch_driver",
            "Board::Display::hres == 800U",
            "Board::Display::vres == 480U",
            "Board::Display::rgb565",
            "Board::Lcd::db0",
            "Board::Lcd::db15",
            "Board::Lcd::hsync",
            "Board::Lcd::vsync",
            "Board::Lcd::hen",
            "Board::Lcd::pclk",
            "Board::Lcd::cs",
            "Board::Lcd::mosi",
            "Board::Lcd::sck",
            "Korvo1LcdGraph",
            "Korvo1StrapGraph",
            "Board::Lcd::db15 == Board::Strap::lcd_db15",
            "Board::Lcd::cs == Board::Strap::b0",
            "Board::Lcd::pclk == Board::Strap::b2",
            "Board::Lcd::mosi == Board::Strap::b3",
            "Board::Lcd::sck == Board::Strap::b4",
            "strap_edges",
            "lcd_probe_frame",
        ),
        problems,
        "ESP32-S31 LCD driver contract",
    )
    check_contains(
        example_root / "ptp" / "main" / "app_main.cpp",
        (
            f'#include "{S31_BOARD_HEADER}"',
            "!Board::Onboard::eth_phy",
            "external_phy",
        ),
        problems,
        "ESP32-S31 PTP external PHY contract",
    )
    check_contains(
        example_root / "ml" / "main" / "app_main.cpp",
        (
            "arc::soc::Target::simd",
            "arc::Soc::simd",
            "arc::ml::saturate_s8",
        ),
        problems,
        "ESP32-S31 ML SDK SIMD contract",
    )
    check_contains(
        example_root / "radio" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core espnow ble_mesh thread)",),
        problems,
        "ESP32-S31 radio main CMake",
    )
    check_contains(
        example_root / "radio" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_ESPNOW_CONTRACT",
            "arc::ble::Mesh::provision",
            "arc::ble::Mesh::publish",
            "arc::net::Thread::attach",
            "arc::net::Thread::send",
            "Board::Wireless::wifi6",
            "Board::Wireless::ble",
            "Board::Wireless::bt54",
            "Board::Wireless::bt_classic",
            "Board::Wireless::ieee802154",
            "Board::Wireless::zigbee3",
            "Board::Wireless::thread14",
            "Board::Wireless::pcb_antenna",
            "Board::Wireless::wifi6 == arc::soc::Target::wifi6",
            "Board::Wireless::ble == arc::soc::Target::ble",
            "Board::Wireless::bt54 == arc::soc::Target::bt54",
            "Board::Wireless::bt_classic == arc::soc::Target::bt_classic",
            "Board::Wireless::ieee802154 == arc::soc::Target::ieee802154",
            "arc::Soc::wifi && arc::Soc::ble && arc::Soc::ble_mesh",
        ),
        problems,
        "ESP32-S31 radio stack contract",
    )
    check_contains(
        example_root / "sd" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core gpio sd)",),
        problems,
        "ESP32-S31 SD main CMake",
    )
    check_contains(
        example_root / "sd" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_SD_DRIVER_CONTRACT",
            "using CardCtrl",
            "CardCtrl::mask()",
            "using Storage",
            "arc::Soc::sdmmc",
            "Board::Resource::SdSlot",
            "Storage::Resource",
            "Board::Resource::SdCtrl",
            "Board::Onboard::microsd",
            "Board::Sd::width == 4U",
            "Board::Sd::sdio3",
            "Board::Sd::audio_store",
            "Board::Sd::playback",
            "!Board::Onboard::spi_nand",
            "Board::SpiNand::shares_sd",
            "Board::SpiNand::requires_rework",
            "Board::SpiNand::supports_1v8",
            "Board::SpiNand::supports_3v3",
            "Board::SpiNand::remove_count == 6U",
            "Board::SpiNand::base_count == 9U",
            "Board::SpiNand::v18_count == 7U",
            "Board::SpiNand::v33_count == 1U",
            "Board::Sd::d0 == Board::SpiNand::clk",
            "Board::Sd::cmd == Board::SpiNand::wp",
            "Board::Sd::ctrl == 39",
            "Board::Sd::ctrl == Board::Strap::b1",
            "Korvo1NandGraph",
            "Korvo1StrapGraph",
            "strap_edges",
            "nand_shared",
        ),
        problems,
        "ESP32-S31 SD driver contract",
    )
    check_contains(
        example_root / "security" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core secure_boot puf cloak)",),
        problems,
        "ESP32-S31 security main CMake",
    )
    check_contains(
        example_root / "security" / "main" / "app_main.cpp",
        (
            "arc::secure::SecureBoot::state",
            "arc::secure::SecureBoot::digest",
            "arc::secure::SecureBoot::revoke",
            "arc::WorldGuard",
            "arc::crypto::Puf::von_neumann",
            "arc::crypto::Puf::derive_with",
            "arc::crypto::Cloak::scramble",
            "arc::soc::Target::secure_boot",
            "arc::soc::Target::flash_encryption",
            "arc::soc::Target::tee",
            "arc::soc::Target::puf",
            "arc::soc::Target::worldguard",
            "arc::soc::has<arc::soc::Cap::tee>",
            "arc::soc::has<arc::soc::Cap::world>",
            "Board::Module::flash_mb == 16U",
            "Board::Module::psram_mb == 16U",
        ),
        problems,
        "ESP32-S31 security contract",
    )
    check_contains(
        example_root / "usb" / "main" / "CMakeLists.txt",
        ("arc_requires(main_requires core otg usb_device usb_host)",),
        problems,
        "ESP32-S31 USB main CMake",
    )
    check_contains(
        example_root / "usb" / "main" / "app_main.cpp",
        (
            "ARC_S31_KORVO_USB_PHY_CONTRACT",
            "using UsbPhy",
            "arc::Soc::usb_otg",
            "UsbPhy::Resource",
            "Board::Resource::UsbOtg",
            "std::is_same_v<Board::Resource::UsbOtg, UsbPhy::Resource>",
            "Board::Usb::dp_module_pin == 40",
            "Board::Usb::dm_module_pin == 41",
            "Board::Usb::dp_gpio == -1",
            "Board::Usb::dm_gpio == -1",
            "usb_dp_module_pin",
            "usb_dm_module_pin",
            "static_assert(!Board::Usb::module_pins_are_gpio)",
            "static_assert(!Board::pins::has<Board::Usb::dp_gpio>())",
            "static_assert(!Board::pins::has<Board::Usb::dm_gpio>())",
            "Board::UsbHost::type_a",
            "Board::UsbHost::high_speed",
            "Board::UsbHost::current_limited",
            "Board::UsbHost::switch_model",
            "Board::UsbHost::downstream_ma == 500U",
            "Board::Power::input_ma == 3'000U",
            "Board::Power::buck_3v3",
            "Board::Power::power_led_5v",
            "arc::usb::DeviceDescriptor",
            "arc::usb::Cdc",
            "arc::usb::HostConfig",
            "Board::Onboard::usb_otg",
        ),
        problems,
        "ESP32-S31 USB driver contract",
    )

    check_contains(
        root / "tests" / "host" / "esp32s31_compile.cpp",
        (
            "#define ARC_TARGET_ESP32S31 1",
            '#include "arc/touch.hpp"',
            "using Board = arc::board::Korvo1",
            "static_assert(arc::soc::s31)",
            "static_assert(arc::Soc::simd)",
            "static_assert(arc::Soc::adc)",
            "static_assert(arc::Soc::rmt)",
            "static_assert(arc::Soc::sdmmc)",
            "static_assert(arc::Soc::usb_otg)",
            "static_assert(!arc::Soc::touch)",
            "static_assert(arc::Soc::wifi)",
            "static_assert(arc::Soc::ble)",
            "static_assert(arc::Soc::ble_mesh)",
            "static_assert(arc::Soc::touch_max == 0U)",
            "static_assert(!arc::soc::has<arc::soc::Cap::amp>)",
        ),
        problems,
        "ESP32-S31 host compile contract",
    )
    check_contains(
        root / "tests" / "host" / "stubs" / "soc" / "soc_caps.h",
        (
            "#define SOC_HOST_TOUCH 0",
            "#define SOC_HOST_TOUCH_MAX_CHAN_ID 0",
            "#define SOC_TOUCH_SENSOR_SUPPORTED SOC_HOST_TOUCH",
            "#define SOC_TOUCH_MAX_CHAN_ID SOC_HOST_TOUCH_MAX_CHAN_ID",
        ),
        problems,
        "ESP32-S31 host touch capability stub",
    )
    check_contains(
        root / "components" / "arc" / "include" / "arc" / "fence.hpp",
        (
            "defined(__riscv)",
            "fence rw, rw",
        ),
        problems,
        "ESP32-S31 RISC-V fence contract",
    )
    check_contains(
        root / "tools" / "compile-fail-check.py",
        (
            "s31_bare_core_rejects_unwired_true_amp",
            "arc::BareCore true AMP is not wired for ESP32-S31",
            "s31_touch_bus_rejects_s3_capacitive_touch",
            "s31_touch_rejects_s3_capacitive_touch",
            "arc::TouchBus is ESP32-S3 capacitive touch only",
            "arc::Touch is ESP32-S3 capacitive touch only",
        ),
        problems,
        "ESP32-S31 negative touch contracts",
    )
    check_contains(
        root / "tests" / "host" / "CMakeLists.txt",
        (
            "examples/esp32s31/${example}/main/app_main.cpp",
            "ARC_TARGET_ESP32S31=1",
            "arc-host-s31-examples",
        ),
        problems,
        "ESP32-S31 example host compile targets",
    )
    check_example_list(
        "tests/host/CMakeLists.txt",
        cmake_s31_examples(root / "tests" / "host" / "CMakeLists.txt"),
        problems,
    )
    check_contains(
        root / "cmake" / "arc-idf.cmake",
        (
            "ARC_EXPERIMENTAL_ESP32S31",
            *S31_SDK_GATE_NEEDLES,
            "complete esp32s31 target metadata",
        ),
        problems,
        "ESP32-S31 CMake target metadata gate",
    )
    check_contains(
        root / "tools" / "arc_idf_test.py",
        (
            "test_rejects_esp32s31_without_experimental_gate",
            "test_rejects_esp32s31_when_idf_lacks_target_metadata",
            "test_accepts_esp32s31_env_gate_with_target_metadata",
        ),
        problems,
        "ESP32-S31 CMake gate tests",
    )
    check_contains(
        root / "tools" / "env_loader_test.py",
        (
            "test_bash_rejects_s31_without_gate_before_loading_idf",
            "test_bash_rejects_s31_with_gate_when_idf_lacks_target",
            "test_bash_accepts_s31_with_gate_and_target_metadata",
            "test_fish_rejects_s31_with_gate_when_idf_lacks_target",
        ),
        problems,
        "ESP32-S31 env loader tests",
    )
    check_contains(
        root / "tools" / "arc_projects.py",
        (
            'rel.startswith("examples/esp32s31/")',
            '"esp32s31", True',
            "preflight_command",
            "tools/s31-readiness.py --idf-path",
            "S31_PREVIEW_IDF_PATH",
            'os.environ.get("S31_PREVIEW_IDF_PATH")',
            'os.environ.get("ARC_IDF_PATH")',
            "tools/s31-build.py --idf-path",
            "--example",
        ),
        problems,
        "ESP32-S31 project discovery",
    )
    check_contains(
        root / "tools" / "s31_build.py",
        (
            "S31_EXAMPLES",
            "s31-readiness.py",
            "ARC_IDF_PATH",
            "ARC_TARGET",
            "ARC_EXPERIMENTAL_ESP32S31",
            "IDF_TARGET",
            "serial_port_key",
            "serial_ports",
            "serial/by-id",
            "resolve_port",
            'parts = ["idf.py", "-C", project]',
            'actions.append("flash")',
            'actions.append("monitor")',
            "examples/esp32s31",
            "--dry-run",
            "--example",
            "--port",
            "--auto-port",
            "--list-ports",
            "--flash",
            "--monitor",
            "requires exactly one --example",
            "ESP32-S31 flash/monitor requires --port or --auto-port",
            "--auto-port is only used with --flash or --monitor",
            "--port and --auto-port are mutually exclusive",
        ),
        problems,
        "ESP32-S31 build driver",
    )
    check_contains(
        root / "tools" / "s31-build.py",
        ("from s31_build import main",),
        problems,
        "ESP32-S31 build driver wrapper",
    )
    check_contains(
        root / "tools" / "s31_build_test.py",
        (
            "test_serial_ports_prefers_stable_by_id_then_usb_devices",
            "test_serial_ports_deduplicates_by_id_aliases",
            "test_auto_port_uses_by_id_alias_when_one_physical_device_exists",
            "test_cli_auto_port_rejects_missing_candidates",
            "test_cli_auto_port_rejects_multiple_candidates",
            "test_cli_flash_requires_explicit_or_auto_port",
            "test_cli_monitor_requires_explicit_or_auto_port",
            "test_cli_auto_port_requires_flash_or_monitor",
            "test_cli_port_and_auto_port_are_mutually_exclusive",
        ),
        problems,
        "ESP32-S31 build driver tests",
    )
    check_contains(
        root / "tools" / "ci-build-plan.py",
        (
            "experimental = [project for project in found if project.experimental]",
            "project = project_for(path, experimental)",
            "include_experimental",
            "--include-experimental",
            "experimental projects are skipped unless --include-experimental",
        ),
        problems,
        "ESP32-S31 CI build planning",
    )
    check_contains(
        root / ".github" / "workflows" / "build.yml",
        (
            "workflow_dispatch:",
            "include_experimental:",
            "idf_ref:",
            "ARC_IDF_EFFECTIVE_REF",
            "github.event.inputs.idf_ref",
            "ARC_INCLUDE_EXPERIMENTAL",
            "ARC_IDF_TARGET_SET",
            "ARC_IDF_INSTALL_TARGETS",
            "esp32s3 esp32s31",
            "plan_args=(--buildable)",
            "plan_args+=(--include-experimental)",
            './tools/ci-build-plan.py "${plan_args[@]}"',
            'fetch --depth 1 origin "$ARC_IDF_EFFECTIVE_REF"',
            "checkout --force FETCH_HEAD",
            "arc-idf-target-set-${ARC_IDF_TARGET_SET}",
            './tools/s31-readiness.py --idf-path "$HOME/esp-idf" --require-sdk',
            '"$HOME/esp-idf/install.sh" "${idf_targets[@]}"',
            "export ARC_TARGET=esp32s31",
            "export ARC_EXPERIMENTAL_ESP32S31=ON",
        ),
        problems,
        "ESP32-S31 workflow opt-in build planning",
    )

    if require_sdk:
        problems.extend(blockers)
    status = "incomplete" if problems else "ready" if not blockers else "blocked"
    return {
        "ok": not problems,
        "status": status,
        "require_sdk": require_sdk,
        "sdk_target": {
            "path": sdk_target["path"],
            "source": sdk_target["source"],
            "available": sdk_target["available"],
            "missing": sdk_target["missing"],
            "checks": sdk_target["checks"],
            "checked": sdk_target["checked"],
        },
        "examples": found_examples,
        "expected_examples": list(EXPECTED_EXAMPLES),
        "summary": {
            "expected_examples": len(EXPECTED_EXAMPLES),
            "found_examples": len(found_examples),
            "problems": len(problems),
            "blockers": len(blockers),
        },
        "problems": problems,
        "blockers": blockers,
    }


def print_report(payload: dict[str, Any]) -> None:
    print("arc ESP32-S31 readiness report")
    print(f"- status: {payload['status']}")
    print(f"- scaffold ok: {'yes' if payload['ok'] else 'no'}")
    print(f"- sdk target: {payload['sdk_target']['path']}")
    print(f"- sdk source: {payload['sdk_target']['source']}")
    print(f"- sdk available: {'yes' if payload['sdk_target']['available'] else 'no'}")
    if payload["sdk_target"].get("missing"):
        print(f"- sdk missing: {', '.join(payload['sdk_target']['missing'])}")
    print(f"- examples: {', '.join(payload['examples']) if payload['examples'] else 'none'}")
    if payload["blockers"]:
        print("- blockers:")
        for blocker in payload["blockers"]:
            print(f"  - {blocker}")
    if payload["problems"]:
        print("- problems:")
        for problem in payload["problems"]:
            print(f"  - {problem}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check Arc ESP32-S31/Korvo scaffold readiness.")
    parser.add_argument("--root", type=Path, default=ROOT, help="Repository root.")
    parser.add_argument(
        "--idf-path",
        type=Path,
        default=None,
        help="ESP-IDF checkout to inspect before ARC_IDF_PATH, IDF_PATH, and repo-local esp-idf.",
    )
    parser.add_argument(
        "--require-sdk",
        action="store_true",
        help="Treat missing local ESP32-S31 ESP-IDF target metadata as a failure.",
    )
    parser.add_argument("--format", choices=OUTPUT_FORMATS, default="text", help="Output style.")
    args = parser.parse_args(argv)

    payload = readiness(args.root.resolve(), require_sdk=args.require_sdk, idf_path=args.idf_path)
    if args.format == "json":
        print(json.dumps(payload, indent=2, sort_keys=True))
    elif args.format == "report":
        print_report(payload)
    elif payload["ok"]:
        print(f"arc ESP32-S31 readiness: {payload['status']}")

    if payload["problems"]:
        if args.format == "text":
            print("\n".join(payload["problems"]), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
