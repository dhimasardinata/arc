# Arc ESP32-S31 Camera Display

Experimental ESP32-S31 scaffold for camera/display DMA planning. It is not part of default CI and requires ESP-IDF target support for `esp32s31`.

Build when a preview ESP-IDF target is available:

```sh
cd examples/esp32s31/cam
export ARC_TARGET=esp32s31
export ARC_EXPERIMENTAL_ESP32S31=ON
. ../../../env.sh
idf.py build
```
