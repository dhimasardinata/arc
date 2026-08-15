# Arc ESP32-S31 AMP Migration

Experimental ESP32-S31 scaffold for Core0 control and Core1 deterministic work partitioning. It keeps true bare-core AMP disabled until an S31 policy exists. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/amp
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example amp --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example amp
```
