# Store Example

This is a standalone ESP-IDF project under `examples/store`.

- The program is written directly in `main/app_main.cpp`.
- It initializes NVS through `arc::Store::boot()`.
- It loads a typed control word from flash with `arc::Store::load_or(...)`.
- It mutates the value and writes it back with `arc::Store::save(...)`.
- It prints flash and heap capacity through `arc::Space`, including current image size vs the active OTA slot.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_store.bin`
- `build/arc_store.elf`

## API Shape

```cpp
ESP_ERROR_CHECK(arc::Store::boot());

esp_err_t load = ESP_OK;
auto cfg = arc::Store::load_or("cfg", "control", boot_cfg, &load);
ESP_ERROR_CHECK(arc::Store::save("cfg", "control", cfg));
```

The main API pieces are:

- `arc::Store::boot()`
- `arc::Store::load(ns, key, value)`
- `arc::Store::load_or(ns, key, fallback)`
- `arc::Store::save(ns, key, value)`
- `arc::Store::erase(ns, key)`
