# Arc ESP32-S31 Korvo USB

Experimental ESP32-S31-Korvo-1 scaffold for USB OTG PHY ownership, the Type-A
500 mA host budget behind the TPS2051C current-limited switch, and class-facing
USB descriptors. It is not part of default CI and requires ESP-IDF target
support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/usb
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example usb --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example usb
```
