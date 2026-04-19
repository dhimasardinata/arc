# Pulse Example

This is a standalone ESP-IDF project under `examples/pulse`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Pulse` to let MCPWM generate a hardware waveform.
- It uses `arc::Capture` to timestamp edges in hardware without a CPU sampling loop.
- The default demo expects a jumper from `GPIO4` to `GPIO5`.

## What It Shows

At runtime, this example does two things:

- route `GPIO4` into an MCPWM waveform at `20 kHz` and `25%` duty
- route `GPIO5` into an MCPWM capture channel and log period/high/low widths in microseconds

No Core 1 loop is needed. The silicon owns both the output and the measurement path.

## Wiring

- Connect `GPIO4` to `GPIO5`
- Share board ground as usual

If your board uses different free pins, edit `out_pin` and `in_pin` in `main/app_main.cpp`.

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

- `build/arc_pulse.bin`
- `build/arc_pulse.elf`

## API Shape

The program surface stays small:

```cpp
using Out = arc::Pulse<4, 20'000, 250>;
using In = arc::Capture<5, 1'000'000>;
```

The main API pieces are:

- `arc::Pulse<Pin, Hz, DutyPermille, Group, ResolutionHz>`
- `arc::Pulse::start()`
- `arc::Pulse::hz(value)`
- `arc::Pulse::duty(value)`
- `arc::Pulse::on()`
- `arc::Pulse::off()`
- `arc::Pulse::wave()`
- `arc::Capture<Pin, Hz, Group, Prescale, Rise, Fall>`
- `arc::Capture::start()`
- `arc::Capture::period()`
- `arc::Capture::high()`
- `arc::Capture::low()`
- `arc::Capture::ticks()`
