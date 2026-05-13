# Sigma Example

This is a standalone ESP-IDF project under `examples/esp32s3/sigma`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Sigma` to drive the ESP32-S3 Sigma-Delta Modulator.
- The peripheral generates the pulse-density stream; the CPU only updates the target density.

## What It Shows

`arc::Sigma<pin, 1'000'000>` binds one GPIO to one SDM output stream at compile time. The example sweeps the density between `-90` and `90`.

The raw output is PDM/SDM. Add a low-pass filter if you want an analog voltage:

```text
Vout ~= VDD_IO / 256 * density + VDD_IO / 2
```

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

- `build/arc_sigma.bin`
- `build/arc_sigma.elf`

## API Shape

```cpp
using Analog = arc::Sigma<4, 1'000'000, 0>;

Analog::start();
Analog::write(64);
Analog::write<-64>();
Analog::zero();
```

Use `Analog::fast(value)` only after `start()` when you intentionally want the thinnest hot-path call into the SDM driver.
