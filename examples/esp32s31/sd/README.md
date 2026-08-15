# Arc ESP32-S31 Korvo SD

Experimental ESP32-S31-Korvo-1 scaffold for the onboard SDIO 3.0 4-bit microSD
audio storage/playback lane, optional SPI NAND alias lane, exact NAND rework
part counts, and SDMMC ownership. It is not part of default CI and requires
ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/sd
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example sd --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example sd
```
