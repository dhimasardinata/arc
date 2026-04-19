# DSP Example

This is a standalone ESP-IDF project under `examples/dsp`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::dsp` plus `arc::simdbuf`.
- Core 1 runs the math loop, Core 0 only reports snapshots.

## What It Shows

At runtime, this example:

- allocates cache-aligned SIMD-capable buffers
- runs `scale`, `mac`, `dot`, `peak`, and an `8-tap` FIR on Core 1
- publishes the latest snapshot through `arc::SeqReg`
- logs the snapshot from Core 0

This is the lane to use when the realtime plane should spend cycles on math rather than transport or GPIO.

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

- `build/arc_dsp.bin`
- `build/arc_dsp.elf`

## API Shape

The program surface stays small:

```cpp
using Core1 = arc::App<Kernel, arc::cfg::stack, arc::Core::core1>;
```

The main API pieces are:

- `arc::simdbuf<T>(n)`
- `arc::dsp::scale(...)`
- `arc::dsp::mac(...)`
- `arc::dsp::dot(...)`
- `arc::dsp::peak(...)`
- `arc::dsp::Fir<T, N>::step(...)`
