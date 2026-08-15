# Arc ESP32-S31 Korvo Console

Experimental ESP32-S31-Korvo-1 scaffold for the board UART0 console lane. It
is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/console
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example console --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example console
```
