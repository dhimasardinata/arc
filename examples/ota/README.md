# OTA Example

This is a standalone ESP-IDF project under `examples/ota`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Ota` to inspect the running slot, boot slot, next update slot, and rollback state.
- It pairs that with `arc::Space` so the partition map and slot capacity stay visible together.
- It does not write or erase flash. This example is intentionally inspection-only.

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

- `build/arc_ota.bin`
- `build/arc_ota.elf`

## API Shape

```cpp
const auto* running = arc::Ota::running();
const auto* next = arc::Ota::next();

esp_ota_img_states_t state{};
ESP_ERROR_CHECK(arc::Ota::state(running, state));
```

The write path is available in the framework as:

- `arc::Ota::Session`
- `arc::Ota::begin(...)`
- `arc::Ota::write(...)`
- `arc::Ota::write_at(...)`
- `arc::Ota::finish(...)`
- `arc::Ota::confirm()`
- `arc::Ota::rollback()`
