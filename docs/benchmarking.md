# Benchmarking

Arc benchmarks are split by evidence level. Host results catch algorithmic regressions early. ESP32-S3 firmware results are the authority for target timing.

## Host Regression Benchmarks

Run:

```sh
./tools/host-bench.sh
```

This benchmark runs Arc host-compilable primitives and standard-library baselines in one binary. It is a regression signal for queue shape, codec work, DSP kernels, and accidental overhead before firmware builds start.

For framework-adjacent host lanes, run:

```sh
./tools/framework-compare.sh
```

That script reports the checked-out ESP-IDF and Arduino-ESP32 source versions, then measures host-buildable Arc, raw ESP-IDF mbedTLS, and Arduino core paths where the same process can execute the compared code honestly.

## ESP32-S3 Firmware Benchmarks

The on-device benchmark is:

```sh
./tools/ensure-frameworks.sh
cd examples/esp32s3/bench
. ../../../env.sh
idf.py build flash monitor
```

The firmware prints grouped `arc-bench` rows with an explicit `surface` column:

- `arc` for Arc primitives, codecs, DSP, service wrappers, and usage mixes.
- `idf` for raw ESP-IDF silicon or library calls measured in the same firmware image.
- `arduino` for optional Arduino-ESP32 component lanes when the local checkout is available.
- `std` for standard C/C++ baselines that are valid on the target.

## Competitive Scope

The benchmark should compare real, like-for-like work:

- queue handoff, fan-in, latest-snapshot publication, and log lanes;
- stream framing and caller-buffer MQTT, WebSocket, and CoAP codecs;
- telemetry bridge, control tick, and protocol-bundle usage mixes;
- DSP dot, scale, MAC, FIR, SOS, state-space, and SIMD-adjacent kernels;
- raw ESP-IDF RNG, SHA, AES/GCM, NVS, OTA, temperature, async memcpy, and TWAI self-test paths;
- optional Arduino `Print`, frame emission, integer formatting, and base64 paths;
- Arc `Text` and `format_to` integer formatting lanes beside the Arduino print lane.

Do not publish fake external-bus numbers. SPI, I2C, UART, ADC, LCD, camera, and wired CAN comparisons need a documented fixture, attached devices, stable wiring, and captured evidence.

## Reporting Rules

Every public benchmark report should include:

- Arc commit SHA;
- ESP-IDF commit/tag;
- Arduino-ESP32 commit/tag when enabled;
- ESP32-S3 board and flash/PSRAM configuration;
- exact command used;
- raw serial output, not only summarized winners.

That keeps benchmark claims reproducible and prevents host-only convenience numbers from being mistaken for silicon timing.
