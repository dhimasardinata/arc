# Arc

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- `arc::Dio` and `arc::Din` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::Ring` handles ordered telemetry.
- `arc::Reg` is the single-word fast path for latest-wins control.

## Layout

```text
.
├── CMakeLists.txt
├── env.sh
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
│               ├── task.hpp
│               └── wave.hpp
└── main
    ├── app.hpp
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

## Outputs

- App binary: `build/arc.bin`
- ELF: `build/arc.elf`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`

Arc uses one build directory by default: `build/`.
If you keep a separate release profile in parallel, use `build-release/` deliberately.

## Demo Topology

The shipped app is intentionally asymmetric:

- Core 1 drives `arc::Dio`
- Core 1 samples `arc::Din`
- Core 1 pushes edge telemetry through `arc::Ring`
- Core 0 drains telemetry every `1 ms`
- Core 0 updates the latest blink rate through `arc::Reg`

`main/app_main.cpp` stays minimal:

```cpp
#include "app.hpp"

extern "C" void app_main()
{
    app::boot();
}
```

## Notes

- Global ESP-IDF is fully supported and is usually the cleaner setup.
- Local `esp-idf/` is still useful when you want a pinned checkout beside the firmware repo.
- `arc::Dio` is the default CPU-driven output path on ESP32-S3.
- `arc::Din` gives the same dedicated path for deterministic input sampling.
- `arc::Reg` is better than a queue for latest-wins control words.
- `arc::Ring` is better than a register when event history matters.
