# Temp Example

This is a standalone ESP-IDF project under `examples/temp`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Temp` to read the ESP32-S3 internal temperature sensor.
- The sensor belongs on Core 0 telemetry/control code, not in a Core 1 hot loop.

## What It Shows

`arc::Temp<-10, 80>` installs the hardware temperature sensor once, enables it, and exposes readings as Celsius or milli-Celsius.

Use this as a thermal guard lane when the firmware pushes CPU frequency, radio, DMA, and peripheral throughput hard.

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

- `build/arc_temp.bin`
- `build/arc_temp.elf`

## API Shape

```cpp
using Die = arc::Temp<-10, 80>;

Die::start();
auto c = Die::celsius();
auto mc = Die::milli();
```
