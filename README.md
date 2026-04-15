# Arc

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- User programs live in `main/app_main.cpp`.
- `arc::Dio` and `arc::Din` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::Sketch` runs a tiny zero-cost program on a chosen core.
- `arc::Bus` gives shared event/control state without heap or virtual dispatch.
- `arc::net::Udp` is a reusable Core 0 transport plane, not an example-only wrapper.

## Layout

```text
.
├── CMakeLists.txt
├── env.sh
├── examples
│   └── udp
│       └── README.md
├── README.md
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

The shipped baseline is a tiny mirror program:

- `arc::Din` samples the configured input pin.
- `arc::Dio` drives the configured LED pin.
- Core 1 mirrors input level to output level with no queue, no heap, and no RTOS API in the hot loop.

`main/app_main.cpp` looks like this:

```cpp
#include "arc.hpp"

using Sense = arc::Din<arc::cfg::sense, 0>;
using Led = arc::Dio<arc::cfg::led, 0>;

struct Mirror {
    static void setup()
    {
        Sense::in();
        Led::out();
        Led::template write<false>();
    }

    IRAM_ATTR static void loop() noexcept
    {
        if (Sense::high()) {
            Led::hi();
        } else {
            Led::lo();
        }
    }
};

using Core1 = arc::Sketch<Mirror, arc::cfg::stack, arc::Core::core1>;

namespace app {
inline void boot() { Core1::boot("arc"); }
}

extern "C" void app_main()
{
    app::boot();
}
```

The core clue is explicit where it matters:

- `arc::Core::core1` in `arc::Sketch<...>` means the hot loop is pinned to Core 1.
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

`env.sh` works in this order:

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

Release with PSRAM:

```bash
idf.py fullclean
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.psram;sdkconfig.defaults.release" set-target esp32s3 build
```

## Command Reference

Build:

```bash
idf.py build
```

Flash:

```bash
idf.py -p /dev/ttyACM0 flash
```

Erase flash:

```bash
idf.py -p /dev/ttyACM0 erase-flash
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

### `arc::Dio<Pin, Channel, Core>`

Dedicated GPIO output bound at compile time.

- `out()` routes the pin to dedicated output.
- `hi()` and `lo()` are the hot-path writes.
- `write<true>()` and `write<false>()` are zero-cost compile-time forms.

### `arc::Din<Pin, Channel, Core>`

Dedicated GPIO input bound at compile time.

- `in()` routes the pin to dedicated input.
- `high()` and `low()` read the dedicated input register directly.

### `arc::Sketch<Program, StackBytes, Core>`

The smallest way to run a program on one core.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void loop() noexcept`

`Sketch` turns that into a static pinned task without heap allocation.

### `arc::Bus<Event, Control, Capacity>`

Shared state for asymmetric programs.

- `events` is an `arc::Ring<Event, Capacity>`
- `pace` is an `arc::Reg<std::uint32_t>`
- `control` is an `arc::Reg<Control>`

Use it when Core 1 emits ordered events and Core 0 applies latest-wins control.

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

The example writes the program directly in `examples/udp/main/app_main.cpp`.

- `Bus = arc::Bus<Edge, Control, 256>`
- `Core1 = arc::Plane<Pulse, ... , Bus>`
- `Core0 = arc::net::Udp<Udp, Bus>`

This is the intended style for larger apps: the app composes framework utilities in `main.cpp`, while the heavy transport/runtime machinery stays inside `arc`.

## Notes

- Global ESP-IDF is fully supported and is usually the cleaner setup.
- Local `esp-idf/` is still useful when you want a pinned checkout beside the firmware repo.
- `arc::Dio` is the default CPU-driven output path on ESP32-S3.
- `arc::Din` gives the same dedicated path for deterministic input sampling.
- `arc::Reg` is better than a queue for latest-wins control words.
- For multi-field control words, prefer explicit fixed-width fields over C++ bitfields.
- `arc::Ring` is better than a register when event history matters.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
