# I2S Example

This is a standalone ESP-IDF project under `examples/i2s`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::I2s`.
- The default demo is a duplex loopback lane.

## What It Shows

At runtime, this example:

- allocates one standard-mode I2S controller
- preloads TX DMA
- streams TX and RX through the hardware DMA path
- logs RX bytes and DMA event counters

This is the lane to use when audio or framed sample streams should move through the I2S engine instead of a software copy loop.

## Wiring

Make one loopback jumper:

- `GPIO11 -> GPIO12`

Default pins:

- `GPIO9` = BCLK
- `GPIO10` = WS
- `GPIO11` = DOUT
- `GPIO12` = DIN

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

- `build/arc_i2s.bin`
- `build/arc_i2s.elf`

## API Shape

The program surface stays small:

```cpp
using Link = arc::I2s<9, 10, 11, 12, 16'000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO, arc::I2sStd::msb>;
```

The main API pieces are:

- `arc::I2s<...>::boot()`
- `arc::I2s<...>::start()`
- `arc::I2s<...>::preload(...)`
- `arc::I2s<...>::write(...)`
- `arc::I2s<...>::read(...)`
- `arc::I2s<...>::sent()`
- `arc::I2s<...>::recv()`
