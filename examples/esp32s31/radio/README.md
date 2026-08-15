# Arc ESP32-S31 Radio Example

Experimental ESP32-S31-Korvo-1 radio scaffold for the ESP32-S31-WROOM-3
module capability facts: Wi-Fi 6, Bluetooth 5.4, Bluetooth Classic, BLE,
IEEE 802.15.4, Zigbee 3.0, Thread 1.4, and PCB antenna routing. The example
uses Arc BLE Mesh and Thread payload validation on host builds and only enables
the ESP-NOW type contract when the preview ESP-IDF headers are present.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/radio
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example radio --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example radio
```
