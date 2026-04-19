# SPI Example

This is a standalone ESP-IDF project under `examples/spi`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::SpiBus` and `arc::Spi`.
- The default demo is a loopback transfer on `SPI2_HOST`.

## What It Shows

At runtime, this example:

- boots a DMA-capable SPI master bus on Core 0
- polls one full-duplex transaction per cycle
- verifies that the received bytes match the transmitted bytes

This is the lane to use when a peripheral should move bytes through the SPI engine instead of CPU bit banging.

## Wiring

Make one loopback jumper:

- `GPIO11 -> GPIO13`

Default bus pins:

- `GPIO12` = SCLK
- `GPIO11` = MOSI
- `GPIO13` = MISO

No CS pin is used in this loopback demo.

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

- `build/arc_spi.bin`
- `build/arc_spi.elf`

## API Shape

The program surface stays small:

```cpp
using Bus = arc::SpiBus<SPI2_HOST, 12, 11, 13>;
using Dev = arc::Spi<Bus, -1, SPI_MASTER_FREQ_20M, 0>;
```

The main API pieces are:

- `arc::SpiBus<...>::boot()`
- `arc::Spi<...>::boot()`
- `arc::Spi<...>::poll(...)`
- `arc::Spi<...>::xfer(...)`
- `arc::Spi<...>::queue(...)`
- `arc::Spi<...>::wait(...)`
