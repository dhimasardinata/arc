# Examples

Each example is a small firmware project with its own `main/`, `CMakeLists.txt`,
and environment wrappers. Build one example at a time while learning so the
logs stay short and the hardware path is obvious.

Use this pattern for ESP32-S3 examples:

```sh
cd examples/esp32s3/udp
. ./env.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Use the matching `source ./env.fish` form if your shell is fish.
Run `./tools/arc-projects.py --buildable --format report` from the repository
root when you want the discovered project list grouped with target, experiment
status, and build command.

## ESP32-S3 Examples

| Example | What it teaches |
| --- | --- |
| `examples/esp32s3/bench` | Real ESP32-S3 benchmark firmware with Arc, raw ESP-IDF, and optional Arduino-ESP32 surfaces in one run. |
| `examples/esp32s3/bridge` | Complementary MCPWM outputs, dead-time, and a small `app::boot()` surface. |
| `examples/esp32s3/can` | TWAI/CAN self-test loopback without an external CAN transceiver. |
| `examples/esp32s3/copy` | DMA memcpy, coherent cache handoff, and exact completion tickets. |
| `examples/esp32s3/count` | RMT pulse output feeding PCNT pulse counting through a jumper. |
| `examples/esp32s3/dsp` | SIMD-capable buffers, DSP kernels, Core 1 compute, and snapshot reporting. |
| `examples/esp32s3/dvp` | LCD_CAM DVP camera-capture skeleton. |
| `examples/esp32s3/espnow` | ESP-NOW raw radio transport with the same Core 0/Core 1 shape as UDP. |
| `examples/esp32s3/i2c` | I2C master bus/device ownership. |
| `examples/esp32s3/i2s` | Duplex I2S DMA movement and `arc::Result<std::size_t>` read/write paths. |
| `examples/esp32s3/i80` | LCD_CAM Intel 8080 DMA display/peripheral output. |
| `examples/esp32s3/net_client` | TCP/TLS/HTTP-style client-side network building blocks. |
| `examples/esp32s3/ota` | OTA slot inspection, image state, and flash capacity reporting. |
| `examples/esp32s3/probe` | Cycle-counter measurement, min/avg/max stats, and Core 1 reporting. |
| `examples/esp32s3/pulse` | MCPWM waveform generation plus MCPWM capture timing. |
| `examples/esp32s3/pwm` | LEDC hardware PWM without a pinned task or busy loop. |
| `examples/esp32s3/scope` | ADC continuous DMA capture and parsed sample frames. |
| `examples/esp32s3/sigma` | Sigma-Delta pulse-density output. |
| `examples/esp32s3/sleep` | RTC-retained boot counter, wake causes, and deep sleep. |
| `examples/esp32s3/space` | Runtime flash, partition, image, and heap capacity reports. |
| `examples/esp32s3/spi` | SPI DMA loopback with coherent queue and finish. |
| `examples/esp32s3/store` | Typed NVS config, fixed text storage, and capacity reporting. |
| `examples/esp32s3/temp` | Internal die-temperature telemetry. |
| `examples/esp32s3/timer` | GPTimer alarm ISR and free-running counter logging. |
| `examples/esp32s3/trace` | RMT transmit/capture loopback for symbol timing. |
| `examples/esp32s3/uart` | UART ownership and byte movement. |
| `examples/esp32s3/udp` | The first full transport example: Core 1 work plus Core 0 Wi-Fi/UDP. |

## Portable Example

| Example | What it teaches |
| --- | --- |
| `examples/portable/pack` | Fixed binary `arc::pack` records outside a full ESP32-S3 firmware build. |

## Experimental ESP32-S31 Scaffolds

These projects are present so the intended API shape is visible, but they stay
gated behind `ARC_TARGET=esp32s31` and `ARC_EXPERIMENTAL_ESP32S31=ON` until a
usable ESP32-S31 ESP-IDF target exists in the pinned SDK.

| Example | Intended surface |
| --- | --- |
| `examples/esp32s31/amp` | AMP-style split-core policy scaffolding. |
| `examples/esp32s31/cam` | Camera/data-path scaffolding. |
| `examples/esp32s31/control` | Control-loop scaffolding. |
| `examples/esp32s31/ml` | Fixed-shape ML scaffolding. |
| `examples/esp32s31/ptp` | Precision-time scaffolding. |

## Reading Order

1. Build `examples/esp32s3/udp` if you want the complete Arc shape.
2. Build the smallest hardware example near your peripheral.
3. Open [Module Guide](/modules) to find the matching header.
4. Open [API Reference](/api) for exact methods and failure behavior.
