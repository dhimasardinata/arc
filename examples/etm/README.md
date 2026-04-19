# ETM Example

This is a standalone ESP-IDF project under `examples/etm`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Route` to connect one GPTimer alarm event to three ETM tasks.
- After boot, the waveform runs in hardware without a callback, queue, or polling loop.

## What It Shows

At runtime, this example does three things in hardware:

- `arc::Alarm<Clock>` emits one ETM event whenever the GPTimer hits its alarm value
- `arc::Toggle<GPIO4>` flips the output pin on every alarm
- `arc::Arm<Clock>` and `arc::Reload<Clock>` re-arm the one-shot alarm path so the timer keeps oscillating without CPU help

The default demo drives `GPIO4` as a `5 kHz` square wave from a `100 us` half-period.

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

- `build/arc_etm.bin`
- `build/arc_etm.elf`

## API Shape

The program surface stays small:

```cpp
using Clock = arc::Timer<1'000'000>;
using Blink = arc::Route<arc::Alarm<Clock>, arc::Toggle<4>>;
using Rearm = arc::Route<arc::Alarm<Clock>, arc::Arm<Clock>>;
using Reset = arc::Route<arc::Alarm<Clock>, arc::Reload<Clock>>;
```

The main API pieces are:

- `arc::Route<Event, Task>`
- `arc::Alarm<Timer>`
- `arc::Arm<Timer>`
- `arc::Reload<Timer>`
- `arc::Toggle<Pin>`
- `arc::Etm::dump()`
