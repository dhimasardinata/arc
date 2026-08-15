# Arc ESP32-S31 PTP

Experimental ESP32-S31 scaffold for PTP discipline logic and future Ethernet timestamping. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

ESP32-S31 has Ethernet MAC capability, but ESP32-S31-Korvo-1 V1.1 does not expose an onboard Ethernet PHY in its board facts. Treat this as an external-PHY scaffold, not a Korvo onboard peripheral bring-up.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/ptp
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example ptp --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example ptp
```
