# Arc ESP32-S31 Korvo IO

Experimental ESP32-S31-Korvo-1 scaffold for the GPIO42 `ADC BUTTON` shared
function buttons and the GPIO37 `WS2812_CTRL` addressable RGB status LED board
facts. It is not part of default CI and requires ESP-IDF target support for
`esp32s31`.

Arc follows the Korvo V1.1 pin assignment table for the status LED: GPIO8 is
LCD DB0 there, while `WS2812_CTRL` is GPIO37.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/io
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example io --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example io
```
