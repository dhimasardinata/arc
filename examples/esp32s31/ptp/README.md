# Arc ESP32-S31 PTP Ethernet

Experimental ESP32-S31 scaffold for Ethernet PTP work. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/ptp
export ARC_TARGET=esp32s31
export ARC_EXPERIMENTAL_ESP32S31=ON
. ../../../env.sh
idf.py build
```
