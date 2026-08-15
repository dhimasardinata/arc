# Arc ESP32-S31 Security Example

Experimental ESP32-S31-Korvo-1 security scaffold for the ESP32-S31 secure boot,
flash encryption, TEE, PUF, and WorldGuard capability facts. The example uses
policy stubs to validate Arc SecureBoot, WorldGuard, PUF extraction, and Cloak
contracts without burning eFuses, changing flash-encryption state, or claiming
runtime hardware security proof.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/security
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 ../../../tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example security --dry-run
python3 ../../../tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example security
```
