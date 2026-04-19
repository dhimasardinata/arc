# Trace Example

This is a standalone ESP-IDF project under `examples/trace`.

- The program is written directly in `main/app_main.cpp`.
- `arc::Burst` emits one short hardware frame on an output GPIO.
- `arc::Trace` captures the returning RMT symbols on another GPIO.
- A tiny Core 0 host task prints the captured pulse widths so you can verify the RMT TX/RX path end to end.

## Wiring

Add a jumper wire:

- `CONFIG_ARC_TRACE_OUT` -> `CONFIG_ARC_TRACE_IN`

By default:

- output = `GPIO4`
- input = `GPIO5`

Change them in `menuconfig` if those pins are not suitable on your board.

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

- `build/arc_trace.bin`
- `build/arc_trace.elf`

## API Shape

```cpp
using Source = arc::Burst<CONFIG_ARC_TRACE_OUT, 1'000'000>;
using Sink = arc::Trace<CONFIG_ARC_TRACE_IN, 1'000'000>;
```

The example arms RX, sends one frame, then inspects the captured symbols:

```cpp
ESP_ERROR_CHECK(Sink::arm());
ESP_ERROR_CHECK(Source::send(frame));
ESP_ERROR_CHECK(Source::wait(100));
```

This keeps the signal generation and timing capture in silicon. The host task only initiates the transaction and prints the resulting widths.
