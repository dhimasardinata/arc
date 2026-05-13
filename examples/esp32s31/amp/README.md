# Arc ESP32-S31 AMP Migration

Experimental ESP32-S31 scaffold for Core0 control and Core1 deterministic work partitioning. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/amp
export ARC_TARGET=esp32s31
export ARC_EXPERIMENTAL_ESP32S31=ON
. ../../../env.sh
idf.py build
```
