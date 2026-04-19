# Count Example

This is a standalone ESP-IDF project under `examples/count`.

- The program is written directly in `main/app_main.cpp`.
- `arc::Burst` emits a repeated hardware pulse train on one GPIO.
- `arc::Count` measures the incoming edges on another GPIO without a CPU polling loop.
- A tiny Core 0 host task reports the accumulated count so you can verify the offload path.

## Wiring

Add a jumper wire:

- `CONFIG_ARC_COUNT_OUT` -> `CONFIG_ARC_COUNT_IN`

By default:

- output = `GPIO4`
- input = `GPIO5`

Change them in `menuconfig` if those pins are not suitable on your board.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_count.bin`
- `build/arc_count.elf`

## API Shape

```cpp
using Source = arc::Burst<CONFIG_ARC_COUNT_OUT, 1'000'000, 64, 1, false>;
using Meter = arc::Count<CONFIG_ARC_COUNT_IN>;
```

The example loops the hardware burst forever:

```cpp
Meter::start();
Source::start();
ESP_ERROR_CHECK(Source::send(frame, -1, true));
```

This means the pulse train keeps running in silicon while the host task only samples the hardware counter.
