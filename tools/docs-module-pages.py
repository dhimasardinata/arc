#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
HEADERS = ROOT / "components" / "arc" / "include" / "arc"
OUT = DOCS / "modules"

SPECIAL_FEATURES = {
    "acoustic_slam": "acoustic_slam",
    "arch-riscv": "core",
    "arch-xtensa": "core",
    "assume": "core",
    "audit": "core",
    "bare_core": "core",
    "bridge": "mcpwm",
    "burst": "rmt",
    "bus": "core",
    "cache_lock": "memory",
    "capture": "mcpwm",
    "cfg": "core",
    "claim": "core",
    "clock": "core",
    "concepts": "core",
    "coro": "core",
    "count": "pcnt",
    "detail-cold": "internal",
    "detail-owner": "internal",
    "drive": "gpio",
    "ecs": "math",
    "fence": "core",
    "flash_log": "store",
    "flash_off": "core",
    "flow": "flow",
    "foc": "mcpwm",
    "fsm": "core",
    "gpio": "gpio",
    "i2c_slave": "i2c",
    "i80": "lcd",
    "init": "core",
    "interrupt_matrix": "core",
    "ip": "net",
    "kalman": "math",
    "log": "core",
    "mask": "core",
    "matrix": "math",
    "motion": "math",
    "mpsc": "core",
    "otg": "otg",
    "pbuf": "poll",
    "pipeline": "memory",
    "place": "memory",
    "plane": "core",
    "pmr": "memory",
    "postmortem": "core",
    "prefetch": "memory",
    "pru": "lcd",
    "pulse": "mcpwm",
    "rcu": "core",
    "reg": "core",
    "result": "core",
    "rgb": "lcd",
    "roles": "core",
    "rpc": "core",
    "rtc_ring": "rtc",
    "rtos": "core",
    "scope": "adc",
    "sdk": "core",
    "sense": "gpio",
    "seq": "core",
    "simd": "math",
    "sketch": "core",
    "soc": "core",
    "soc-esp32s3": "core",
    "soc-esp32s31": "core",
    "soc-target": "core",
    "spi_slave": "spi",
    "spsc": "core",
    "stack": "core",
    "stream": "net_codecs",
    "swarm": "fabric",
    "task": "core",
    "tee": "sandbox",
    "text": "core",
    "tight": "core",
    "timesync": "time",
    "topology": "core",
    "trace": "rmt",
    "trace_event": "core",
    "trace_stream": "trace_live",
    "ulp_asm": "ulp",
    "ulp_cxx": "ulp",
    "usb_device": "usb_device",
    "usb_host": "usb_host",
    "vision": "robotics",
    "vm": "sandbox",
    "watch": "core",
    "wave": "time",
}

EXAMPLES = {
    "adc": "examples/esp32s3/scope",
    "bridge": "examples/esp32s3/bridge",
    "burst": "examples/esp32s3/count",
    "can": "examples/esp32s3/can",
    "capture": "examples/esp32s3/pulse",
    "copy": "examples/esp32s3/copy",
    "count": "examples/esp32s3/count",
    "dsp": "examples/esp32s3/dsp",
    "dvp": "examples/esp32s3/dvp",
    "espnow": "examples/esp32s3/espnow",
    "file": "examples/esp32s3/store",
    "fs": "examples/esp32s3/store",
    "gpio": "examples/esp32s3/pwm",
    "i2c": "examples/esp32s3/i2c",
    "i2c_slave": "examples/esp32s3/i2c",
    "i2s": "examples/esp32s3/i2s",
    "i80": "examples/esp32s3/i80",
    "net": "examples/esp32s3/udp",
    "ota": "examples/esp32s3/ota",
    "pack": "examples/portable/pack",
    "probe": "examples/esp32s3/probe",
    "pulse": "examples/esp32s3/pulse",
    "pwm": "examples/esp32s3/pwm",
    "rgb": "examples/esp32s3/i80",
    "scope": "examples/esp32s3/scope",
    "sd": "examples/esp32s3/store",
    "sigma": "examples/esp32s3/sigma",
    "sleep": "examples/esp32s3/sleep",
    "space": "examples/esp32s3/space",
    "spi": "examples/esp32s3/spi",
    "spi_slave": "examples/esp32s3/spi",
    "store": "examples/esp32s3/store",
    "temp": "examples/esp32s3/temp",
    "timer": "examples/esp32s3/timer",
    "trace": "examples/esp32s3/trace",
    "uart": "examples/esp32s3/uart",
    "udp": "examples/esp32s3/udp",
    "usb": "examples/esp32s3/uart",
    "wave": "examples/esp32s3/pwm",
}

GROUP_EXAMPLES = {
    "Buses, Displays, And Data Capture": "examples/esp32s3/spi",
    "Crypto, Security, VM, And Sandbox": "examples/esp32s3/store",
    "DSP, Control, ML, And Vision": "examples/esp32s3/dsp",
    "GPIO, Timing, And Power": "examples/esp32s3/pwm",
    "Lock-Free Lanes": "examples/esp32s3/probe",
    "Memory, DMA, And Placement": "examples/esp32s3/copy",
    "Network, Radio, And Wire Protocols": "examples/esp32s3/udp",
    "Observability And Trace": "examples/esp32s3/trace",
    "Program Shape And Ownership": ".",
    "Profile Modules": ".",
    "Storage And Update": "examples/esp32s3/store",
    "Target And Naming Contract": ".",
    "ULP And Low-Power Coprocessor": "examples/esp32s3/sleep",
}


def slug_for(rel: str) -> str:
    return rel.removesuffix(".hpp").replace("/", "-")


def title_for(rel: str) -> str:
    return f"`{rel}`"


def read_module_table() -> dict[str, tuple[str, str]]:
    group = "Unsorted"
    result: dict[str, tuple[str, str]] = {}
    for line in (DOCS / "modules.md").read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            group = line.removeprefix("## ").strip()
            continue
        match = re.match(r"\| `([^`]+)` \| (.+) \|$", line)
        if not match:
            continue
        header, purpose = match.groups()
        if header.startswith("arc/") or header == "arc.hpp":
            result[header] = (group, purpose)
    return result


def cmake_features() -> set[str]:
    text = (ROOT / "cmake" / "arc-deps.cmake").read_text(encoding="utf-8")
    features = set(re.findall(r'feature STREQUAL "([^"]+)"', text))
    features.add("core")
    return features


def header_paths() -> list[str]:
    items = ["arc.hpp"]
    items.extend(f"arc/{path.relative_to(HEADERS).as_posix()}" for path in sorted(HEADERS.rglob("*.hpp")))
    return items


def feature_for(rel: str, features: set[str]) -> str:
    if rel == "arc.hpp":
        return "core"
    slug = slug_for(rel.removeprefix("arc/"))
    stem = Path(rel).stem
    if slug in SPECIAL_FEATURES:
        return SPECIAL_FEATURES[slug]
    if stem in features:
        return stem
    return "core"


def example_for(rel: str, group: str) -> str:
    if rel == "arc.hpp":
        return "."
    stem = Path(rel).stem
    return EXAMPLES.get(stem, GROUP_EXAMPLES.get(group, "."))


def simulated_output(rel: str, feature: str, example: str) -> str:
    name = rel.removesuffix(".hpp")
    target = "root" if example == "." else example.rsplit("/", 1)[-1]
    return "\n".join(
        [
            f"I (000) arc-doc: module={name}",
            f"I (001) arc-doc: feature={feature} example={target}",
            "I (002) arc-doc: init policy explicit",
            "I (003) arc-doc: no hidden heap in hot path",
        ]
    )


def page_for(rel: str, group: str, purpose: str, feature: str, example: str) -> str:
    include = '#include "arc.hpp"' if rel == "arc.hpp" else f'#include "{rel}"'
    cmake_expr = (
        "arc_requires(main_requires core)" if feature == "core" else f"arc_requires(main_requires core {feature})"
    )
    feature_line = (
        "This is an internal support header; application code normally reaches it through a public module."
        if feature == "internal"
        else f"Declare the CMake feature with `{cmake_expr}` when this header needs ESP-IDF components beyond the base Arc component."
    )
    build_cmd = "idf.py build" if example == "." else f"idf.py -C {example} build"
    flash_cmd = (
        "idf.py -p /dev/ttyACM0 flash monitor"
        if example == "."
        else f"idf.py -C {example} -p /dev/ttyACM0 flash monitor"
    )
    example_note = (
        "The root project is the smallest place to try this module."
        if example == "."
        else f"The closest shipped example is `{example}`."
    )
    return f"""# {title_for(rel)}

{purpose}

## What It Owns

- Header: `{rel}`
- Module group: {group}
- CMake feature: `{feature}`
- Closest example: `{example}`

{feature_line}

## Start From Zero

1. Create or clone an Arc project.
2. Load the ESP-IDF environment with `. ./env.sh`.
3. Include the module header in the file that owns this hardware or policy lane.
4. Add the matching `arc_requires(...)` feature in that component's `CMakeLists.txt`.
5. Build the closest example first, then move the same pattern into your app.

Minimal component shape:

```cmake
include(${{CMAKE_CURRENT_LIST_DIR}}/../cmake/arc-deps.cmake)

{cmake_expr}

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${{main_requires}}
)
```

```cpp
{include}

namespace app {{
void boot()
{{
    // Keep board policy explicit here. Configure pins, buffers, and ownership
    // before handing work to Core 1 or a Core 0 transport task.
}}
}}

extern "C" void app_main()
{{
    app::boot();
}}
```

## Usage Pattern

- Put topology facts in one board header instead of scattering aliases.
- Keep buffer ownership visible with `std::span`, `std::array`, or Arc capability buffers.
- Use recoverable `init()`, `begin()`, or `set(...)` paths when runtime data can fail.
- Use fail-fast `boot()` or `start()` only for board invariants.
- Keep slow logs, storage, Wi-Fi, and protocol work on Core 0.

## Step-By-Step Check

1. Decide whether this module owns silicon, memory, protocol bytes, or policy only.
2. Name the owner type once, close to the board topology.
3. Allocate any DMA or shared buffers before the hardware starts.
4. Initialize with the recoverable path while bringing up the board.
5. Switch to the fail-fast path only after the topology is treated as fixed.
6. Log from Core 0 after the hot path has handed off a compact event or snapshot.

## Example

{example_note}

```sh
. ./env.sh
{build_cmd}
{flash_cmd}
```

If the example needs a jumper, sensor, display, or external bus device, read the
example README before flashing. The command above proves the build path; real
electrical output still depends on the attached board.

## Simulated Output

This is a documentation simulation, not a captured hardware log:

```text
{simulated_output(rel, feature, example)}
```

## Next Reading

- [Module Guide](/modules)
- [API Reference](/api)
- [Examples](/examples)
"""


def write_pages() -> None:
    table = read_module_table()
    features = cmake_features()
    OUT.mkdir(parents=True, exist_ok=True)
    by_group: dict[str, list[tuple[str, str]]] = {}

    for rel in header_paths():
        group, purpose = table.get(
            rel,
            ("Unsorted", "Arc public header. Use the API reference and source comments for the exact contract."),
        )
        slug = slug_for(rel)
        feature = feature_for(rel, features)
        example = example_for(rel, group)
        (OUT / f"{slug}.md").write_text(
            page_for(rel, group, purpose, feature, example),
            encoding="utf-8",
        )
        by_group.setdefault(group, []).append((rel, slug))

    lines = [
        "# Module Page Index",
        "",
        "Every public Arc header has a generated website page with purpose, CMake feature, zero-start steps, usage pattern, example command, and simulated output.",
        "",
    ]
    for group in sorted(by_group):
        lines.append(f"## {group}")
        lines.append("")
        for rel, slug in sorted(by_group[group]):
            lines.append(f"- [`{rel}`](/modules/{slug})")
        lines.append("")
    (DOCS / "module-pages.md").write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    write_pages()
