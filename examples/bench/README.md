# Arc ESP32-S3 Benchmark

On-device benchmark for Arc hot primitives on a real ESP32-S3, plus same-firmware framework comparisons where the path is valid on hardware without an external fixture.

It measures runtime on the target using the Xtensa cycle counter and prints cycles/op for:

- `Spsc` single and batch transfers
- `Mpsc` and `DenseMpsc`
- `Fanin` single and batch transfers
- `SeqReg`
- `Stream` write, frame encode, and frame read
- MQTT, WebSocket, and CoAP caller-buffer codecs
- critical usage mixes that combine queues, snapshots, DSP, and protocol framing:
  - Core 1 style telemetry batches handed to a Core 0 consumer through `Spsc`
  - a control-loop tick with DSP math, `SeqReg` latest-state publication, and `Fanin` events
  - a telemetry protocol bundle covering MQTT, CoAP, WebSocket, and `frame16` emission
- DSP dot/scale/mac/FIR/peak kernels
- standard `memcpy` and Arc async-DMA copy on the same 256-byte payload
- hardware RNG
- accelerated SHA-256
- AES ECB, CBC, CTR, OFB, and GCM encryption paths
- internal temperature sensor reads
- OTA running/boot/next slot inspection and state lookup
- typed NVS blob/string save and load paths
- TWAI/CAN self-test loopback send/receive on the on-chip controller

The firmware now also compares Arc against other framework surfaces on the same ESP32-S3 run:

- raw ESP-IDF silicon APIs for RNG, PSA SHA-256, `esp_aes_*`, `esp_aes_gcm_*`, and async memcpy
- Arduino-ESP32 core paths for `Print::write`, manual `frame16` emission, integer print formatting, and `base64::encode`

It still does not publish fake numbers for SPI/I2C/UART/ADC/LCD/etc. Those need a board fixture, attached devices, and a stable wiring policy to be meaningful. The CAN lane here is strictly the ESP32-S3 self-test loopback path, not a benchmark for a real external bus.

Arduino coverage is optional. The bench auto-enables it when one of these exists:

- sibling checkout at `../../arduino-esp32`
- `ARC_ARDUINO_PATH=/abs/path/to/arduino-esp32`

The NVS write lanes use only `16` rounds per run so the one-shot benchmark does not hammer flash unnecessarily.

The benchmark has its own `sdkconfig.defaults` overlay that gives `app_main` an 8 KiB stack and keeps FreeRTOS stack canaries enabled. Large benchmark work buffers live in static storage rather than on `main_task`, and the firmware prints stack high-water marks after each section so stack pressure is visible next to timing data.

Run:

```sh
# optional: fetch/update the Arduino checkout used by the inter-framework leg
./tools/ensure-frameworks.sh

cd examples/bench
. ../../env.sh
idf.py build flash monitor
```

If you do not want the Arduino leg, skip `./tools/ensure-frameworks.sh` and run the same `idf.py` flow; the firmware will still publish Arc plus raw ESP-IDF comparisons.

Look for `arc-bench` log lines. Output is grouped by benchmark area. Each lane prints total operations, cycles per operation, and nanoseconds per operation for the real ESP32-S3 run. Arc lanes keep their original names, raw ESP-IDF baselines are prefixed with `idf`, Arduino-ESP32 baselines are prefixed with `arduino`, and scenario lanes are prefixed with `usage`.
