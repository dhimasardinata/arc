# Probe Example

This is a standalone ESP-IDF project under `examples/probe`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Probe` to read the Xtensa cycle counter around a hot path.
- It uses `arc::CycleStats` to keep min/avg/max cycle counts without heap allocation.

## What It Shows

At runtime, Core 1 repeatedly measures:

- `arc::Drive::on()`
- `arc::Drive::off()`

Core 0 logs the min/avg/max cycle count through an `arc::SeqReg` snapshot. This is the audit lane for Arc hot paths: measure the claim, do not guess.

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

- `build/arc_probe.bin`
- `build/arc_probe.elf`

## API Shape

The program surface stays small:

```cpp
const auto mark = arc::Probe::now();
Led::on();
Led::off();
cycles.add(mark.lap());
```

The main API pieces are:

- `arc::Probe::now()`
- `arc::Probe::lap()`
- `arc::CycleStats::add(cycles)`
- `arc::CycleStats::avg()`
