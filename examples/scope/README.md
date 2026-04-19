# Scope Example

This is a standalone ESP-IDF project under `examples/scope`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Scope` to stream ADC samples through the ESP32-S3 digital controller and DMA path.
- The default demo samples `GPIO4` as `ADC1_CHANNEL_3`.

## What It Shows

At runtime, this example does three things:

- start a `40 kHz` ADC continuous stream on `GPIO4`
- pull parsed samples from the driver without byte-level parsing in user code
- log `avg/min/max` plus frame/overflow counters

This is the lane to use when analog throughput matters and CPU polling is waste.

## Wiring

- Connect an analog source or potentiometer to `GPIO4`
- Keep the signal inside the ESP32-S3 ADC voltage range
- Share board ground normally

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_scope.bin`
- `build/arc_scope.elf`

## API Shape

The program surface stays small:

```cpp
using In = arc::Scope<40'000, 256, 4096, true, arc::Adc<4>>;
```

The main API pieces are:

- `arc::Adc<Io, Atten, Width>`
- `arc::Scope<Hz, FrameBytes, StoreBytes, Flush, Pads...>`
- `arc::Scope::start()`
- `arc::Scope::pull(...)`
- `arc::Scope::raw(...)`
- `arc::Scope::frames()`
- `arc::Scope::overruns()`
