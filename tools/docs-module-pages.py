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
    "axi_graph": "memory",
    "bare_core": "core",
    "bft": "net_codecs",
    "borrow": "core",
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
    "crdt": "net_codecs",
    "detail-cold": "internal",
    "detail-owner": "internal",
    "detail-quant": "internal",
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
    "proof": "core",
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
    "vision_accel": "examples/esp32s3/dvp",
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

SPECIAL_LANDMARKS = {
    "arc/borrow.hpp": [
        "BorrowMode",
        "StaticRef",
        "StaticLoan",
        "StaticRead",
        "StaticMut",
        "StaticReads",
        "StaticWrites",
        "LoanPack",
        "HasLoan",
        "HasStaticRead",
        "HasStaticWrite",
        "StaticReadable",
        "StaticWritable",
        "LoanReadable",
        "LoanWritable",
    ],
    "arc/detail/quant.hpp": ["mul_sat", "add_sat", "round_shift_s64"],
    "arc/migrator.hpp": [
        "FleetPeer",
        "IdleCore",
        "AnyIdleCore",
        "WasmSnapshot",
        "MigrationDecision",
        "MigrationFrame",
        "Migrator",
    ],
    "arc/hls.hpp": ["KernelSpec", "SiliconTarget", "SiliconPlan", "StaticLoop", "DenseSpec", "Interface"],
    "arc/plane.hpp": ["Plane", "StaticPlane", "BareWork", "BoundWork"],
    "arc/proof.hpp": ["Kind", "Claim", "Fact", "ProofFact", "Pack"],
}

GROUP_GUIDANCE = {
    "Buses, Displays, And Data Capture": {
        "when": "a real ESP32-S3 peripheral needs one visible owner for pins, DMA descriptors, and transfer lifetime",
        "avoid": "the code only needs a protocol codec or a board-independent data transform",
        "check": "prove pin mapping, DMA-capable buffers, queue/finish ordering, and attached-device wiring before treating runtime output as evidence",
    },
    "Crypto, Security, VM, And Sandbox": {
        "when": "security-sensitive bytes need caller-owned buffers, explicit hardware-policy boundaries, or bounded sandbox hooks",
        "avoid": "key custody, trust decisions, product authorization, or threat modeling belongs above Arc",
        "check": "document key source, buffer lifetime, failure handling, and which side owns the final security decision",
    },
    "DSP, Control, ML, And Vision": {
        "when": "fixed-shape compute must run without dynamic framework allocation in the realtime lane",
        "avoid": "a model, matrix, or vision pipeline still has runtime-varying shapes that need a higher-level planner first",
        "check": "pin input/output sizes, align hot buffers, and benchmark on ESP32-S3 before publishing timing claims",
    },
    "Detail Headers": {
        "when": "you are maintaining Arc internals and need the small support type behind a public module",
        "avoid": "application firmware is choosing a public API; start from the owning public header instead",
        "check": "keep direct use inside Arc code and preserve the public module contract that depends on this helper",
    },
    "GPIO, Timing, And Power": {
        "when": "pin edges, timing sources, sleep state, watchdogs, or power locks must be explicit at the call site",
        "avoid": "a slow policy task can own the work without a realtime hardware contract",
        "check": "verify target clock source, interrupt context, wake source, and board-level pin safety before flashing",
    },
    "Lock-Free Lanes": {
        "when": "Core 1 or an ISR-adjacent lane needs bounded handoff without FreeRTOS queue allocation",
        "avoid": "the producer/consumer shape is not fixed yet or blocking semantics are acceptable",
        "check": "name the producer and consumer, size the queue from worst-case bursts, and drain logs on Core 0",
    },
    "Memory, DMA, And Placement": {
        "when": "a pointer crosses cache, DMA, PSRAM, flash mapping, or another core boundary",
        "avoid": "ordinary stack/local data stays inside one task and never crosses a hardware owner",
        "check": "confirm alignment, capability flags, cache ownership, and completion tickets before reusing buffers",
    },
    "Network, Radio, And Wire Protocols": {
        "when": "Core 0 owns the radio/socket work and payload bytes still need deterministic framing or parsing",
        "avoid": "Core 1 would block on Wi-Fi, TLS, DNS, storage, or heap-heavy service code",
        "check": "keep radio state on Core 0, bound payload buffers, and capture real serial/network output for protocol evidence",
    },
    "Observability And Trace": {
        "when": "runtime evidence must leave the hot path as compact events and be formatted later",
        "avoid": "the hot loop would format strings, allocate JSON, or block on a sink",
        "check": "prove the producer only writes bounded records and the drain path owns formatting, files, UDP, or WebSocket output",
    },
    "Profile Modules": {
        "when": "a reader wants one coherent entry point for a domain or a subset build wants a profile root",
        "avoid": "one source file only needs a narrow peripheral or codec header",
        "check": "confirm the profile does not drag domain roots into small substrate builds unless that is intentional",
    },
    "Program Shape And Ownership": {
        "when": "firmware structure, task lifetime, ownership, command parsing, or policy state needs a visible contract",
        "avoid": "a hardware-specific module already owns the same decision more directly",
        "check": "put the owner near board topology, make rollback explicit, and keep slow side effects on Core 0",
    },
    "Storage And Update": {
        "when": "flash, files, NVS, OTA, or capacity checks must stay recoverable and Core 0-owned",
        "avoid": "a realtime loop would wait on flash, VFS, or partition work",
        "check": "record partition assumptions, rollback behavior, and the exact failure path before shipping update logic",
    },
    "Target And Naming Contract": {
        "when": "code needs compile-time target facts, SDK naming boundaries, or public naming rules",
        "avoid": "runtime board wiring or product policy can express the decision more clearly",
        "check": "keep ESP32-S3 as the stable default and gate ESP32-S31 paths through Arc target selection only",
    },
    "ULP And Low-Power Coprocessor": {
        "when": "tiny retained-state policy must keep running while the main cores sleep",
        "avoid": "the job needs heap, Wi-Fi, storage, or broad C++ runtime support",
        "check": "verify RTC memory layout, wake source, retained data alignment, and the main-core handoff after wake",
    },
}

DEFAULT_GUIDANCE = {
    "when": "the module purpose matches the firmware boundary you are trying to make explicit",
    "avoid": "another narrower Arc header owns the same boundary with less surface area",
    "check": "build the closest example, then capture the real board evidence that matches the module purpose",
}

SPECIAL_GUIDANCE = {
    "arc/vision_accel.hpp": {
        "when": "ESP32-P4 PPA, JPEG, or H264 paths need fixed frame and bitstream plans before board policy submits work",
        "avoid": "SDK handle lifetime, queue depth, interrupts, or cache maintenance policy are still undefined",
        "check": "prove input/output sizes and policy calls on host, then benchmark on ESP32-P4 before publishing timing claims",
    },
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


def header_file(rel: str) -> Path:
    if rel == "arc.hpp":
        return ROOT / "components" / "arc" / "include" / "arc.hpp"
    return ROOT / "components" / "arc" / "include" / rel


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
    raise RuntimeError("module pages use real build/runtime check guidance instead of simulated logs")


def public_landmarks(rel: str) -> list[str]:
    if rel in SPECIAL_LANDMARKS:
        return SPECIAL_LANDMARKS[rel]

    text = header_file(rel).read_text(encoding="utf-8")
    names: list[str] = []
    patterns = [
        r"^\s*(?:template\s*<[^;{}]+>\s*)?(?:class|struct)\s+([A-Z][A-Za-z0-9_]*)\b",
        r"^\s*(?:enum\s+class|enum)\s+([A-Z][A-Za-z0-9_]*)\b",
        r"^\s*using\s+([A-Z][A-Za-z0-9_]*)\b",
    ]
    for pattern in patterns:
        for match in re.finditer(pattern, text, flags=re.MULTILINE):
            name = match.group(1)
            if name not in names:
                names.append(name)
            if len(names) >= 10:
                return names
    if names:
        return names

    includes: list[str] = []
    for match in re.finditer(r'#include\s+"(arc/[^"]+)"', text):
        include = match.group(1)
        if include not in includes:
            includes.append(include)
        if len(includes) >= 8:
            break
    return includes


def feature_requirements(feature: str) -> str:
    if feature in {"core", "internal"}:
        return "arc_requires(main_requires core)"
    return f"arc_requires(main_requires core {feature})"


def render_bullets(items: list[str]) -> str:
    return "\n".join(f"- {item}" for item in items)


def page_for(rel: str, group: str, purpose: str, feature: str, example: str) -> str:
    include = '#include "arc.hpp"' if rel == "arc.hpp" else f'#include "{rel}"'
    cmake_expr = feature_requirements(feature)
    guidance = SPECIAL_GUIDANCE.get(rel, GROUP_GUIDANCE.get(group, DEFAULT_GUIDANCE))
    landmarks = public_landmarks(rel)
    landmark_line = (
        "Source landmarks: " + ", ".join(f"`{name}`" for name in landmarks) + "."
        if landmarks
        else "This header is mostly compile-time policy or forwarding glue; use the purpose and group contract here, then open the source for the exact inline contract."
    )
    is_internal = group == "Detail Headers" or feature == "internal"
    feature_line = (
        "This is Arc implementation support. Application firmware should enter through the public module that owns the behavior."
        if is_internal
        else f"Declare `{cmake_expr}` in the component that includes this header."
    )
    example_note = (
        "The root project is the smallest place to try this module."
        if example == "."
        else f"The closest shipped example is `{example}`."
    )
    include_section = (
        "Application code should not include this detail header directly. Keep direct includes inside Arc internals, tests, or a public wrapper that preserves the public contract."
        if is_internal
        else f"""```cmake
include(${{CMAKE_CURRENT_LIST_DIR}}/../cmake/arc-deps.cmake)

{cmake_expr}

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${{main_requires}}
)
```

```cpp
{include}
```"""
    )
    start_steps = [
        "Start from the closest example or the root project listed below.",
        "Load the ESP-IDF environment with `. ./env.sh`.",
        "Add the include and CMake feature only in the component that owns this lane.",
        "Keep board topology, buffers, and ownership in one visible owner type.",
        "Move from build proof to hardware proof only after the wiring or runtime dependency is known.",
    ]
    if is_internal:
        start_steps = [
            "Find the public Arc module that uses this helper.",
            "Change the public wrapper or test first, then adjust the detail helper.",
            "Keep the helper small enough that application code still has a public entry point.",
            "Run the docs generator and host checks after the internal contract changes.",
        ]
    if is_internal:
        owner_section = f"""The owner is the public Arc wrapper or test that reaches this helper. Do not add a
new application-facing dependency on `arc/detail/...`.

```cpp
// Inside Arc internals or a focused test only.
{include}
```"""
        step_check = """1. Name the public header that depends on this helper.
2. Keep the helper hidden behind that public header.
3. Add or update the host test that exercises the public behavior.
4. Regenerate module pages so this detail page stays synchronized.
5. Run the repository checks before publishing."""
        example_note = "Use host checks for this internal helper before any firmware build."
        command_block = """```sh
python3 tools/docs_module_pages_test.py
./tools/host-tests.sh
```"""
        runtime_check = """This page does not create a user-facing runtime surface. Runtime proof belongs to
the public module or example that owns the behavior using this helper."""
    else:
        build_cmd = "idf.py build" if example == "." else f"idf.py -C {example} build"
        flash_cmd = (
            "idf.py -p /dev/ttyACM0 flash monitor"
            if example == "."
            else f"idf.py -C {example} -p /dev/ttyACM0 flash monitor"
        )
        owner_section = """```cpp
namespace app {
void boot()
{
    // Put board policy, buffer ownership, and failure handling here.
    // Keep Core 1 hot work separate from Core 0 service work.
}
}

extern "C" void app_main()
{
    app::boot();
}
```"""
        step_check = """1. Decide whether this module owns silicon, memory, protocol bytes, or policy only.
2. Name the owner type once, close to the board topology.
3. Allocate any DMA or shared buffers before the hardware starts.
4. Initialize with the recoverable path while bringing up the board.
5. Switch to the fail-fast path only after the topology is treated as fixed.
6. Log from Core 0 after the hot path has handed off a compact event or snapshot."""
        command_block = f"""```sh
. ./env.sh
{build_cmd}
{flash_cmd}
```"""
        runtime_check = """The build command proves the dependency path. Runtime proof still needs the
actual board condition that matches this module: attached device, loopback,
radio peer, flash partition, sleep wake source, or captured serial/network
output. Do not turn the example command into a performance or hardware claim
without that evidence."""

    return f"""# {title_for(rel)}

{purpose}

## Fit

- Use it when {guidance["when"]}.
- Do not start here when {guidance["avoid"]}.
- Verification focus: {guidance["check"]}.

## Arc Contract

- Header: `{rel}`
- Module group: {group}
- CMake feature: `{feature}`
- Closest example: `{example}`

{feature_line}

## CMake And Include

{include_section}

## Source Landmarks

{landmark_line}

## Start From Zero

{render_bullets(start_steps)}

## Owner Skeleton

{owner_section}

## Step-By-Step Check

{step_check}

## Build Or Example

{example_note}

{command_block}

## Runtime Check

{runtime_check}

## Next Reading

- [Module Guide](/modules)
- [API Reference](/api)
- [Examples](/examples)
"""


def write_pages() -> None:
    table = read_module_table()
    features = cmake_features()
    headers = header_paths()
    missing = [rel for rel in headers if rel not in table]
    if missing:
        names = ", ".join(missing)
        raise SystemExit(f"docs/modules.md is missing public header entries: {names}")

    OUT.mkdir(parents=True, exist_ok=True)
    by_group: dict[str, list[tuple[str, str]]] = {}

    for rel in headers:
        group, purpose = table[rel]
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
        "Every public Arc header has a generated website page with purpose, fit, CMake feature, source landmarks, zero-start steps, and the closest build or runtime proof path.",
        "",
        "Use this page when you know the header name. Use the [Module Guide](/modules) when you only know the problem area.",
        "",
    ]
    for group in sorted(by_group):
        lines.append(f"## {group}")
        lines.append("")
        lines.append("| Header | Purpose | Page |")
        lines.append("| --- | --- | --- |")
        for rel, slug in sorted(by_group[group]):
            purpose = table[rel][1]
            lines.append(f"| `{rel}` | {purpose} | [Open](/modules/{slug}) |")
        lines.append("")
    (DOCS / "module-pages.md").write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    write_pages()
