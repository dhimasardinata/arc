# Space Example

This is a standalone ESP-IDF project under `examples/space`.

- The program is written directly in `main/app_main.cpp`.
- It reports flash, partition, and heap capacity through `arc::Space`.
- It then allocates explicit internal, PSRAM, DMA, and SIMD blocks with Arc capability helpers.
- It prints heap capacity again so you can see the effect immediately on real hardware.
- Defaults target `ESP32-S3 N16R8` with `16 MB` flash and `8 MB` Octal PSRAM.

## What It Shows

At runtime, this example reports:

- flash chip size
- running app slot size
- current image size inside the active app slot
- percent used and free inside the active app slot
- percent used across the combined OTA app area
- full partition table with address and size
- general 8-bit heap
- internal heap
- DMA-capable heap
- SIMD-capable heap
- IRAM-capable heap
- RTC RAM heap
- executable-capable heap when the target exposes it
- PSRAM heap
- the delta after explicit internal, PSRAM, DMA, and SIMD allocations

This complements `idf.py size`, which only reports static ELF sections and does not know how much of the active OTA slot is left at runtime.

## Build And Run

From this directory:

```bash
. ./env.sh
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
    dma = arc::dmabuf<std::uint8_t>(dma_bytes);
    simd = arc::simdbuf<std::uint16_t>(simd_words);
    arc::Space::heap("arc-space", "after explicit allocations");
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
- `arc::dmabuf<T>(count)`
- `arc::simdbuf<T>(count)`
- `ARC_DMA`
- `ARC_RTC_NOINIT`
