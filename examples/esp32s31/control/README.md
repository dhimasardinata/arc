# Arc ESP32-S31 Realtime Control

Experimental ESP32-S31 scaffold for deterministic control loops. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/control
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example control --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example control
```
