# Arc ESP32-S31 Korvo Audio

Experimental ESP32-S31-Korvo-1 scaffold for audio codec pin planning, dual
analog microphone speech/wake facts, two NS4150B PA chips, 2 mm speaker
connector pitch, and DMA buffer ownership. It is not part of default CI and
requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/audio
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio
python3 ../../../tools/s31-build.py --list-ports
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor
```
