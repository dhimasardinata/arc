# Arc

Arc is an ESP-IDF base for ESP32-S3 work that wants explicit control over core affinity, memory placement, and hot-path cost.

The split is intentional:

- Core 0 handles framework work: boot, storage, drivers, network, logs.
- Core 1 runs the realtime plane as a statically allocated pinned task.
- `arc::Dio` and `arc::Din` bind the dedicated GPIO instruction path directly to compile-time types.
- `arc::Ring` is the lock-free lane between the two cores.

## Layout

```text
.
├── CMakeLists.txt
├── env.sh
├── README.md
├── sdkconfig.defaults
├── sdkconfig.defaults.psram
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
- `gnu++26` on ESP-IDF v6.0
- C++ exceptions and RTTI disabled
- FreeRTOS trace/stat formatting disabled
- Core 1 idle task removed from the task watchdog
- Static FreeRTOS task allocation for the realtime plane
- Dedicated GPIO output and input on the hot path
- Lock-free SPSC rings for Core 1 -> Core 0 telemetry and Core 0 -> Core 1 commands
- Optional PSRAM overlay for boards that actually have PSRAM

## Bootstrap

Arc expects a local `esp-idf/` checkout next to the project:

```bash
git clone --branch v6.0 --recursive https://github.com/espressif/esp-idf.git
./esp-idf/install.sh esp32s3
```

Then load the environment from the project root:

```bash
. ./env.sh
```

## Commands

Set the target once in a fresh workspace:

```bash
idf.py set-target esp32s3
```

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

If your board has PSRAM:

```bash
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.psram" set-target esp32s3 build
```

## Outputs

- App binary: `build/arc.bin`
- ELF: `build/arc.elf`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`

## Demo Topology

The shipped demo is intentionally asymmetric and complete:

- Core 1 drives `arc::Dio` and samples `arc::Din` inside the steady-state loop.
- Core 1 never blocks. If telemetry cannot be pushed, it is dropped.
- Core 0 polls the telemetry ring every `1 ms`, drains it in bursts, and logs what it sees.
- Core 0 also injects a rate command every `2 s` to exercise the command lane.

`main/app_main.cpp` stays minimal:

```cpp
#include "app.hpp"

extern "C" void app_main()
{
    app::boot();
}
```

## Notes

- This repo uses one build directory: `build/`.
- If you still have `build-esp32s3/`, it is an old artifact from an earlier target-scoped build and can be deleted.
- `arc::Dio` is the right default for CPU-driven output on ESP32-S3. `arc::Gpio` remains available when you want direct MMIO without consuming a dedicated channel.
- `arc::Din` gives you deterministic input sampling on the same dedicated path.
- The realtime contract is on the steady-state loop, not on a half-IRAM bootstrap fantasy.
- Core 0 polling is the right tradeoff here. Jitter is accepted on Core 0 so Core 1 does not pay for RTOS queues, notifications, or locks.
