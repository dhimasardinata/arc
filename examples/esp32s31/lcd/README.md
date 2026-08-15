# Arc ESP32-S31 Korvo LCD

Experimental ESP32-S31-Korvo-1 scaffold for the ESP32-S3-LCD-EV-Board-SUB3
ST7262E43/GT1151 4.3-inch 800x480 RGB565 LCD lane. It is not part of default
CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/lcd
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example lcd --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example lcd
```
