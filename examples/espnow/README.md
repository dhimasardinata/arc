# ESP-NOW Example

This is a standalone ESP-IDF project under `examples/espnow`.

- The program is written directly in `main/app_main.cpp`.
- Core 1 owns the deterministic edge generator and pushes telemetry into `arc::Link`.
- Core 0 owns the raw Wi-Fi radio task through `arc::net::EspNow`.
- The example uses a broadcast peer by default, so it can start without extra provisioning.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_espnow.bin`
- `build/arc_espnow.elf`

## API Shape

```cpp
using Link = arc::Link<Edge, Control, 256>;
using Core0 = arc::net::EspNow<Radio, Link>;
using Core1 = arc::Plane<Pulse, CONFIG_ARC_ESPNOW_STACK, Link>;
```

`Radio` supplies:

- `tag`
- `stack`
- `channel`
- `peer`
- optional `start(bus)`
- optional `tick(bus, now)`
- optional `recv(bus, peer, data, len)`

The default example keeps receive logic out of the hot path and only uses `start(...)` plus `tick(...)`.
