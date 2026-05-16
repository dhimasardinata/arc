# Store Example

This is a standalone ESP-IDF project under `examples/esp32s3/store`.

- The program is written directly in `main/app_main.cpp`.
- It initializes NVS through `arc::Store::boot()`.
- It loads a typed control word from flash with `arc::Store::load_or(...)`.
- It mutates the value and writes it back with `arc::Store::save(...)`.
- It writes and reads fixed-buffer text config through `arc::Store::save_text<N>(...)` and `load_text<N>(...)`.
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
ESP_ERROR_CHECK(arc::Store::save_text<16>("cfg", "name", "arc-n16r8"));

auto name = arc::Store::load_text<16>("cfg", "name", "arc-default");
```

The main API pieces are:

- `arc::Store::boot()`
- `arc::Store::load(ns, key, value)`
- `arc::Store::load_or(ns, key, fallback)`
- `arc::Store::save(ns, key, value)`
- `arc::StoreText<N>`
- `arc::Store::save_text<N>(ns, key, value)`
- `arc::Store::load_text<N>(ns, key, fallback)`
- `arc::Store::save_string(ns, key, value)`
- `arc::Store::string_size(ns, key, bytes)`
- `arc::Store::load_string(ns, key, span, chars)`
- `arc::Store::erase(ns, key)`
