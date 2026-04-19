# Bridge Example

This is a standalone ESP-IDF project under `examples/bridge`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Bridge` to drive a complementary MCPWM pair with explicit dead-time.
- The default demo drives `GPIO4` and `GPIO5` as a half-bridge style output pair.

## What It Shows

At runtime, this example does one thing:

- configure a `20 kHz` complementary pair
- hold `40%` duty on the high-side waveform
- inject `50` ticks rising-edge dead-time and `100` ticks falling-edge dead-time

This is the lane you use when LEDC is no longer enough and the waveform has power-stage consequences.

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

- `build/arc_bridge.bin`
- `build/arc_bridge.elf`

## API Shape

The program surface stays small:

```cpp
using Half = arc::Bridge<4, 5, 20'000, 400, 50, 100>;
```

The main API pieces are:

- `arc::Bridge<HighPin, LowPin, Hz, DutyPermille, RiseDelayTicks, FallDelayTicks>`
- `arc::Bridge::start()`
- `arc::Bridge::hz(value)`
- `arc::Bridge::duty(value)`
- `arc::Bridge::wave()`
- `arc::Bridge::hi()`
- `arc::Bridge::lo()`
- `arc::Bridge::off()`
