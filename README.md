# Arc

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

The checked-in defaults are now tuned for `ESP32-S3 N16R8`:

- `16 MB` Quad SPI flash
- `8 MB` Octal PSRAM
- dual OTA app slots sized for a `16 MB` device

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- User programs live in `main/app_main.cpp`.
- `arc::Drive` and `arc::Sense` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::App` runs a tiny zero-cost program on a chosen core.
- `arc::Link` gives shared event/control state without heap or virtual dispatch.
- `arc::Space` reports runtime flash, partition, and heap capacity without heap allocation.
- `arc::net::Udp` is a reusable Core 0 transport plane, not an example-only wrapper.

## Layout

```text
.
├── CMakeLists.txt
├── env.fish
├── env.sh
├── examples
│   ├── space
│   │   └── README.md
│   └── udp
│       └── README.md
├── README.md
├── partitions_16mb.csv
├── sdkconfig.defaults
├── sdkconfig.defaults.psram
├── sdkconfig.defaults.release
├── components
│   └── arc
│       ├── CMakeLists.txt
│       └── include
│           ├── arc.hpp
│           └── arc
│               ├── bus.hpp
│               ├── caps.hpp
│               ├── cfg.hpp
│               ├── clock.hpp
│               ├── din.hpp
│               ├── dio.hpp
│               ├── fence.hpp
│               ├── gpio.hpp
│               ├── plane.hpp
│               ├── reg.hpp
│               ├── ring.hpp
│               ├── sketch.hpp
│               ├── space.hpp
│               ├── task.hpp
│               ├── udp.hpp
│               └── wave.hpp
└── main
    ├── app_main.cpp
    ├── CMakeLists.txt
    └── Kconfig.projbuild
```

## What Is Tuned

- `-O2` via `CONFIG_COMPILER_OPTIMIZATION_PERF`
- CPU default frequency forced to `240 MHz`
- `gnu++26`
- C++ exceptions and RTTI disabled
- Xtensa-safe `-mtext-section-literals` for C++ units that host IRAM code
- `16 MB` flash header and partition layout for `ESP32-S3 N16R8`
- `8 MB` Octal PSRAM at `80 MHz`, exposed explicitly through caps allocation
- Static FreeRTOS task allocation for the realtime plane
- Core 1 idle watchdog detached so a non-yielding loop can own that core
- Dedicated GPIO output and input on the hot path
- Lock-free telemetry ring and single-word control register

## Programming Model

Arc should feel like a small framework, not like a bag of headers.

- The user-facing program goes in `main/app_main.cpp`.
- `app::boot()` is the normal signature entry you expose for your program.
- `extern "C" void app_main()` stays as the single ESP-IDF boundary.
- For a trivial app, `app::boot()` may be the only thing `app_main()` does.
- For a richer app, `main/app_main.cpp` still shows the program topology directly.

The shipped baseline is a tiny follow program:

- `arc::Sense` samples the configured input pin.
- `arc::Drive` drives the configured LED pin.
- Core 1 mirrors input level to output level with no queue, no heap, and no RTOS API in the hot loop.

`main/app_main.cpp` looks like this:

```cpp
#include "arc.hpp"

using Button = arc::Sense<arc::cfg::sense, 0>;
using Led = arc::Drive<arc::cfg::led, 0>;

struct Follow {
    static void setup()
    {
        Button::in();
        Led::out();
        Led::off();
    }

    IRAM_ATTR static void loop() noexcept
    {
        if (Button::high()) {
            Led::on();
        } else {
            Led::off();
        }
    }
};

using Core1 = arc::App<Follow, arc::cfg::stack, arc::Core::core1>;

namespace app {
inline void boot() { Core1::boot("arc"); }
}

extern "C" void app_main()
{
    app::boot();
}
```

The core clue is explicit where it matters:

- `arc::Core::core1` in `arc::App<...>` means the hot loop is pinned to Core 1.
- `arc::net::Udp` always owns Core 0 when you use it.

## Start From Zero

### 1. Install prerequisites

You need at minimum:

- `git`
- `python3`
- a working serial permission setup for your board

ESP-IDF will install its own toolchain and Python environment with `install.sh`.

### 2. Create your own project from the template

Web:

- Open `https://github.com/dhimasardinata/arc`
- Click `Use this template`

CLI:

```bash
gh repo create my-fw --template dhimasardinata/arc --private
git clone https://github.com/<you>/my-fw.git
cd my-fw
```

If you just want to inspect Arc directly:

```bash
git clone https://github.com/dhimasardinata/arc.git
cd arc
```

## Install ESP-IDF

Arc supports both global and project-local ESP-IDF.

### Option A: global ESP-IDF

This is the better default if you work on more than one ESP-IDF repo.

```bash
mkdir -p ~/esp
cd ~/esp
git clone --branch v6.0 --recursive https://github.com/espressif/esp-idf.git
./esp-idf/install.sh esp32s3
```

Then expose it in your shell:

```bash
export IDF_PATH="$HOME/esp/esp-idf"
```

You can put that line in `.bashrc` or `.zshrc`.

### Option B: project-local ESP-IDF

Use this if you want the toolchain and IDF checkout pinned next to the repo.

```bash
git clone --branch v6.0 --recursive https://github.com/espressif/esp-idf.git esp-idf
./esp-idf/install.sh esp32s3
```

## Load the environment

From the project root:

```bash
. ./env.sh
```

For fish:

```fish
source ./env.fish
```

The env loader works in this order:

- `ARC_IDF_PATH`
- existing global `IDF_PATH`
- local `./esp-idf`

## First Build

Set the target once in a fresh workspace:

```bash
idf.py set-target esp32s3
```

Build the default dev profile:

```bash
idf.py build
```

Those defaults already assume `ESP32-S3 N16R8`. The optional `sdkconfig.defaults.psram` file is kept only for backward compatibility with older commands.

Flash and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Release Profile

Arc ships a separate release overlay in `sdkconfig.defaults.release`.

It is more aggressive than the default profile:

- app logs off
- bootloader logs off
- runtime log level control off
- tag-level cache/list off
- `esp_err_to_name()` lookup tables off
- silent assertions/check macros
- silent reboot panic handler

If you are switching from the default profile to release in the same `build/` directory, clean first:

```bash
idf.py fullclean
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.release" set-target esp32s3 build
```

If you want to keep dev and release artifacts side by side, use a second explicit build directory:

```bash
idf.py -B build-release -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.release" set-target esp32s3 build
```

Release:

```bash
idf.py fullclean
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.release" set-target esp32s3 build
```

## Command Reference

Build:

```bash
idf.py build
```

Size summary:

```bash
idf.py size
```

Per-component size:

```bash
idf.py size-components
```

Per-file size:

```bash
idf.py size-files
```

Build and size summary:

```bash
idf.py build size
```

Build and per-component size:

```bash
idf.py build size-components
```

Build and per-file size:

```bash
idf.py build size-files
```

Build, size, and upload:

```bash
idf.py -p /dev/ttyACM0 build size flash
```

Build, size, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build size flash monitor
```

Build, per-file size, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build size-files flash monitor
```

Flash:

```bash
idf.py -p /dev/ttyACM0 flash
```

Erase flash:

```bash
idf.py -p /dev/ttyACM0 erase-flash
```

Decode the built partition table:

```bash
python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin
```

Monitor:

```bash
idf.py -p /dev/ttyACM0 monitor
```

Build and upload:

```bash
idf.py -p /dev/ttyACM0 build flash
```

Upload and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Build, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build flash monitor
```

## API

### `arc::Drive<Pin, Channel, Core>`

Dedicated GPIO output bound at compile time.

- `out()` routes the pin to dedicated output.
- `on()` and `off()` are the clearest hot-path writes.
- `hi()` and `lo()` remain available as low-level synonyms.
- `write<true>()` and `write<false>()` are zero-cost compile-time forms.

`arc::Dio` remains available as a compatibility alias.

### `arc::Sense<Pin, Channel, Core>`

Dedicated GPIO input bound at compile time.

- `in()` routes the pin to dedicated input.
- `high()` and `low()` read the dedicated input register directly.

`arc::Din` remains available as a compatibility alias.

### `arc::App<Program, StackBytes, Core>`

The smallest way to run a program on one core.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void loop() noexcept`

`App` turns that into a static pinned task without heap allocation.

`arc::Sketch` remains available as a compatibility alias.

### `arc::Link<Event, Control, Capacity>`

Shared state for asymmetric programs.

- `events` is an `arc::Ring<Event, Capacity>`
- `pace` is an `arc::Reg<std::uint32_t>`
- `control` is an `arc::Reg<Control>`

Use it when Core 1 emits ordered events and Core 0 applies latest-wins control.

`arc::Bus` remains available as a compatibility alias.

### `arc::Space`

Runtime capacity reporter.

- `flash(tag)` reports flash chip size and the running app slot.
- `parts(tag)` reports every partition with address and size.
- `heap(tag)` reports 8-bit, internal, DMA, IRAM-capable, optional exec, and PSRAM heap capacity.
- `all(tag)` runs all three in one call.

Use this alongside `idf.py size`, `idf.py size-components`, and `idf.py size-files` when you want the real board view.

### `arc::Ring<T, Capacity>`

Single-producer single-consumer event lane.

- use for history you cannot collapse
- push/pop are lock-free and power-of-two indexed

### `arc::Reg<T>`

Single-word latest-wins lane.

- use for control words or pace values
- `T` must fit in 32 bits and be trivially copyable

### `arc::Plane<Work, StackBytes, State, Core>`

Pinned static task for a bound workload.

Your workload defines either:

- `setup()` and `run() noexcept`

or:

- `setup(state)` and `run(state) noexcept`

Use this when you want explicit stateful realtime workers instead of the simpler `Sketch`.

### `arc::net::Udp<Policy, Bus>`

Reusable Core 0 UDP transport plane.

`Policy` supplies compile-time config:

- `tag`
- `stack`
- `ssid`
- `pass`
- `host`
- `port`

Optional hooks:

- `start(bus)`
- `tick(bus, now)`

This lets you keep network code in the framework while still expressing program-specific control in the app.

### `arc::Wave<Pin, HalfUs, Mhz>`

Static square-wave generator when you want a fixed compile-time waveform.

## Outputs

- App binary: `build/arc.bin`
- ELF: `build/arc.elf`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`
- Custom partition CSV: `partitions_16mb.csv`

Arc uses one build directory by default: `build/`.
If you keep a separate release profile in parallel, use `build-release/` deliberately.

## UDP Example

Arc ships a standalone network demo at `examples/udp`.

```bash
cd examples/udp
. ./env.sh
idf.py set-target esp32s3
idf.py menuconfig
idf.py build flash monitor
```

For fish:

```fish
cd examples/udp
source ./env.fish
idf.py set-target esp32s3
idf.py menuconfig
idf.py build flash monitor
```

The example is tuned to the same `N16R8` target and uses its own `examples/udp/partitions_16mb.csv`.

The example writes the program directly in `examples/udp/main/app_main.cpp`.

- `Link = arc::Link<Edge, Control, 256>`
- `Core1 = arc::Plane<Pulse, ... , Link>`
- `Core0 = arc::net::Udp<Udp, Link>`

This is the intended style for larger apps: the app composes framework utilities in `main.cpp`, while the heavy transport/runtime machinery stays inside `arc`.

## Space Example

Arc also ships a standalone capacity demo at `examples/space`.

```bash
cd examples/space
. ./env.sh
idf.py set-target esp32s3
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/space
source ./env.fish
idf.py set-target esp32s3
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows two things:

- baseline runtime capacity via `arc::Space::all(...)`
- the effect of `arc::inram` and `arc::psram` on free heap

## Notes

- Global ESP-IDF is fully supported and is usually the cleaner setup.
- Local `esp-idf/` is still useful when you want a pinned checkout beside the firmware repo.
- Default config assumes `ESP32-S3 N16R8` with `16 MB` flash and `8 MB` Octal PSRAM.
- If your board LED is not on `GPIO48`, change `CONFIG_ARC_LED` or `CONFIG_ARC_UDP_LED` in `menuconfig`.
- `arc::Drive` is the default CPU-driven output path on ESP32-S3.
- `arc::Sense` gives the same dedicated path for deterministic input sampling.
- `arc::Reg` is better than a queue for latest-wins control words.
- For multi-field control words, prefer explicit fixed-width fields over C++ bitfields.
- `arc::Ring` is better than a register when event history matters.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
