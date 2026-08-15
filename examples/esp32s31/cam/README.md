# Arc ESP32-S31 Camera Display

Experimental ESP32-S31 scaffold for camera/display DMA planning, camera 3.3 V
to 2.8 V/1.5 V LDO rail facts, the 800x480 RGB565 LCD bridge, and JPEG
video-streaming facts. It is not part of default CI and requires ESP-IDF target
support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/cam
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example cam --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example cam
```
