# Space Example

This is a standalone ESP-IDF project under `examples/space`.

- The program is written directly in `main/app_main.cpp`.
- It reports flash, partition, and heap capacity through `arc::Space`.
- It then allocates one block explicitly in internal RAM and one block explicitly in PSRAM with `arc::inram` and `arc::psram`.
- It prints heap capacity again so you can see the effect immediately on real hardware.
- Defaults target `ESP32-S3 N16R8` with `16 MB` flash and `8 MB` Octal PSRAM.

## What It Shows

At runtime, this example reports:

- flash chip size
- running app slot size
- full partition table with address and size
- general 8-bit heap
- internal heap
- DMA-capable heap
- IRAM-capable heap
- executable-capable heap when the target exposes it
- PSRAM heap
- the delta after explicit internal and PSRAM allocations

This complements `idf.py size`, which only reports static ELF sections.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py set-target esp32s3
idf.py build
idf.py size
idf.py size-components
idf.py size-files
python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py set-target esp32s3
idf.py build
idf.py size
idf.py size-components
idf.py size-files
python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_space.bin`
- `build/arc_space.elf`

## API Shape

The example keeps the program surface tiny:

```cpp
namespace app {

inline void boot()
{
    arc::Space::all("arc-space", "baseline");
    hot = arc::inram<Hot>();
    cold = arc::psram<Cold>();
    arc::Space::heap("arc-space", "after explicit arc::inram + arc::psram");
}

}
```

The main API pieces are:

- `arc::Space::flash(tag)`
- `arc::Space::parts(tag)`
- `arc::Space::heap(tag)`
- `arc::Space::all(tag)`
- `arc::inram<T>()`
- `arc::psram<T>()`
