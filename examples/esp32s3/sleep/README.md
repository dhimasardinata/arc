# Sleep Example

This is a standalone ESP-IDF project under `examples/esp32s3/sleep`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Sleep` to arm timer wake and enter deep sleep.
- It uses `ARC_RTC` state to keep a boot counter across deep sleep resets.

## What It Shows

This is the lowest-waste lane in Arc: do no work when the chip should be asleep.

The example prints the wake-cause bitmask, arms a `2 s` timer wake, enters deep sleep, then boots again with the retained RTC counter.

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

- `build/arc_sleep.bin`
- `build/arc_sleep.elf`

## API Shape

```cpp
ESP_ERROR_CHECK(arc::Sleep::after<2'000'000>());
arc::Sleep::deep();
```

Useful calls:

- `arc::Sleep::cause()`
- `arc::Sleep::woke(ESP_SLEEP_WAKEUP_TIMER)`
- `arc::Sleep::light()`
- `arc::Sleep::deep()`
- `arc::Sleep::keep(domain)`
