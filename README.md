# Arc

[![build](https://github.com/dhimasardinata/arc/actions/workflows/build.yml/badge.svg)](https://github.com/dhimasardinata/arc/actions/workflows/build.yml)
[![license: AGPL-3.0-only](https://img.shields.io/badge/license-AGPL--3.0--only-blue.svg)](LICENSE)
![ESP-IDF v6.0](https://img.shields.io/badge/ESP--IDF-v6.0-E7352C.svg)
![C++ gnu++26](https://img.shields.io/badge/C%2B%2B-gnu%2B%2B26-00599C.svg)
![target ESP32-S3](https://img.shields.io/badge/target-ESP32--S3-222222.svg)

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

License: `AGPL-3.0-only`. Arc is open source, but intentionally strict copyleft: if you distribute modified firmware or run modified network-accessible services built from Arc, keep the corresponding source available under the same license terms.

Arc is not a convenience wrapper around ESP-IDF. It is a typed substrate for firmware that needs deterministic hot loops, explicit DMA/cache ownership, static task memory, and transport/protocol composition without hidden heap policy.

At a glance:

- Target: ESP32-S3, ESP-IDF `v6.0`, C++ `gnu++26`.
- Runtime shape: Core 0 owns control, networking, storage, logs, and framework work; Core 1 owns deterministic compute and GPIO.
- Allocation stance: hot paths use static storage, caller-owned spans, capability-tagged buffers, and no framework-owned protocol heap.
- Error stance: setup can return `esp_err_t` / `arc::Result<T>`; impossible topology and hot-path contract violations fail early.
- Transport stance: TCP, TLS, HTTP/HTTPS, UDP, ESP-NOW, MQTT, WebSocket, and CoAP are composable lanes/codecs, not task-owning application frameworks.

## Why Arc

Arc exists for firmware where "it probably initializes" and "the ISR usually keeps up" are not acceptable engineering contracts.

| Problem | Arc's answer |
| --- | --- |
| Two files accidentally claim the same UART, SPI, I2C, or pin | Compile-time topology checks plus runtime claim tokens reject incompatible aliases before hardware corruption. |
| DMA buffers silently land in the wrong memory | Capability-tagged buffers and cache handoff APIs make SRAM/PSRAM/DMA ownership visible at the call site. |
| Realtime work competes with networking and logs | Core 1 is the deterministic compute plane; Core 0 owns control, transport, storage, and framework work. |
| Shared peripheral lifetime becomes informal | `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, and `arc::RefLease` encode init, rollback, references, and teardown in one atomic word. |
| Lock-free code accidentally invokes C++ data-race UB | Queue and snapshot lanes use explicit acquire/release sequencing, cache-line layout, and dual-buffer handoff where payload tearing would otherwise be undefined. |
| ESP-IDF APIs leak raw handles into app code | Arc keeps ownership typed, static, and explicit while still returning `esp_err_t` or `arc::Result<T>` when recovery matters. |

The checked-in defaults are now tuned for `ESP32-S3 N16R8`:

- `16 MB` Quad SPI flash
- `8 MB` Octal PSRAM
- dual OTA app slots sized for a `16 MB` device

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- User programs live in `main/app_main.cpp`.
- `arc.hpp` is lean by default and only exposes feature headers whose backing ESP-IDF components are actually in the build graph.
- `cmake/arc-deps.cmake` maps Arc feature names to ESP-IDF components so each app can stay explicit without writing a long `REQUIRES` list by hand.
- `arc::Soc` exposes the ESP32-S3 capability map from `soc_caps.h` as compile-time constants and hard-fails on non-S3 targets.
- `arc::Drive` and `arc::Sense` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::Pins` gives one-file board topology checks so duplicate physical pins are caught where hardware truth is declared.
- `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, and `arc::Gate` provide one-word initialization and shared-resource lifetime state machines for static drivers.
- `arc::Result<T>` is an opt-in `std::expected<T, esp_err_t>` alias for runtime operations that should not hard-panic.
- `arc::Cache` makes DMA/PSRAM cache coherency explicit at the call site, while `arc::Copy::send_coherent(...)` and `finish_coherent(...)` cover the common async-memcpy handoff without blocking useful CPU work.
- `arc::Can` binds the ESP32-S3 TWAI/CAN controller with ISR-backed RX handoff.
- `arc::Burst` streams prebuilt RMT symbols with optional hardware looping.
- `arc::Trace` captures RMT symbols back into SRAM without a CPU sampling loop.
- `arc::Trax` controls Xtensa TRAX instruction trace memory for on-core execution history when TRAX is enabled in Kconfig.
- `arc::Pulse` uses MCPWM for higher-grade waveform generation than LEDC when period and edge placement matter.
- `arc::Bridge` drives complementary MCPWM pairs with explicit dead-time and optional hardware brake input.
- `arc::Capture` timestamps edges in hardware through the MCPWM capture block.
- `arc::AdcBus`, `arc::AdcOne`, and `arc::Scope` cover calibrated ADC oneshot reads and continuous DMA capture without mixing the ownership models.
- `arc::Copy` offloads memory movement to the async DMA memcpy engine.
- `arc::Dvp` captures parallel camera frames through the ESP32-S3 LCD_CAM DMA path.
- `arc::I80Bus` and `arc::I80` drive the ESP32-S3 LCD_CAM Intel 8080 DMA path with exact transfer tickets.
- `arc::Rgb` binds the ESP32-S3 RGB LCD engine with explicit timing, frame-buffer ownership, and refresh control.
- `arc::I2cBus`, `arc::I2c`, and `arc::I2cSlave` bind ESP32-S3 I2C master/slave buses and devices with recoverable init paths.
- `arc::SpiBus`, `arc::Spi`, and `arc::SpiSlave` drive DMA-capable SPI master/slave transfers with ticketed queue/finish ownership.
- `arc::I2s` owns standard-mode I2S channels, `arc::I2sTdm` covers multichannel framed lanes, and `arc::I2sPdm` covers one-line PDM RX/TX, with both raw `esp_err_t` and opt-in `arc::Result` APIs.
- `arc::Uart` binds ESP32-S3 UART ports, pins, framing, and buffers with fixed storage and typed read/write APIs.
- `arc::Usb` binds the ESP32-S3 USB Serial/JTAG lane with typed byte IO.
- `arc::Otg` owns the native USB OTG PHY for host/device stack bring-up without pretending to be a USB class framework.
- `arc::Sd` mounts ESP32-S3 SD/MMC cards through FAT and keeps raw sector access explicit.
- `arc::Fs` mounts SPIFFS and FAT-on-flash VFS paths with fixed handle ownership.
- `arc::File` gives RAII VFS/POSIX file I/O without leaking `FILE*` ownership into app code.
- `arc::Count` offloads pulse accumulation to the PCNT block.
- `arc::dsp` adds hot-loop math kernels that pair naturally with `arc::simdbuf` and Core 1.
- `arc::Probe` reads the Xtensa cycle counter so hot paths can be measured, not guessed.
- `arc::Mask` gives an explicit Core-local interrupt barrier when you really need to silence OS-visible interrupts around a tiny hot section.
- `arc::Pwm` binds LEDC hardware PWM directly to compile-time pin/frequency defaults with runtime duty retuning.
- `arc::Sigma` binds the ESP32-S3 Sigma-Delta Modulator to a compile-time GPIO and sample rate with runtime density retuning.
- `arc::Timer` binds the GPTimer block to a compile-time timebase and optional ISR hook.
- `arc::Sleep` wraps wake causes, timer wake, power-domain policy, light sleep, and deep sleep.
- `arc::Pm` gives typed ESP-IDF PM locks for CPU/APB/no-light-sleep critical sections when DFS is enabled.
- `arc::Rng` exposes the ESP32-S3 hardware random source for fixed buffers and typed values.
- `arc::Time` reads the global SYSTIMER-backed microsecond clock for cross-core timestamps.
- `arc::Sha` hashes buffers through Espressif's accelerated PSA/mbedTLS SHA path with `arc::Result` output.
- `arc::Aes` and `arc::Gcm` wrap AES block/stream/GCM operations with explicit key setup and caller-owned buffers.
- `arc::Hmac` computes eFuse-keyed SHA-256 HMACs and gates temporary JTAG unlock through the HMAC peripheral.
- `arc::Sign` drives the Digital Signature peripheral for eFuse-key-backed RSA signatures.
- `arc::Mpi` owns mbedTLS big integers and uses ESP-IDF's MPI/RSA accelerator path when the target Kconfig enables it.
- `arc::Xts` exposes hardware encrypted flash reads/writes and encrypted-partition alignment checks.
- `arc::Wdt` exposes explicit task-watchdog configuration, task/user subscription, and feeding.
- `arc::Fuse` reads eFuse fields, blocks, MACs, package, and secure-version state.
- `arc::Ulp` loads and runs ULP RISC-V or FSM binaries and gives RTC-shared atomic words for main-core handoff.
- `arc::Temp` reads the ESP32-S3 internal temperature sensor for thermal telemetry.
- `arc::TouchBus` and `arc::Touch` bind the ESP32-S3 capacitive touch controller and channels with explicit scan, filter, wake, and channel-data ownership.
- `arc::Tight` runs a masked per-step loop with optional cycle-budget overrun telemetry for the rare path that needs tighter jitter than `arc::App`.
- `arc::App` runs a tiny zero-cost program on a chosen core.
- `arc::Link` gives shared event/control state without heap or virtual dispatch.
- `arc::Spsc` gives a bounded lock-free lane for one producer and one consumer; `arc::Ring` remains the terse compatibility alias.
- `arc::Audit<Topology>` adds opt-in ownership assertions when you want topology misuse to fail fast instead of staying purely contractual.
- `arc::Mpsc` gives bounded lock-free fan-in when several producers must feed one Core 0 consumer; `arc::Mux` is the terse topology alias.
- `arc::DenseMpsc` keeps the same algorithm but packs cells tightly when RAM density matters more than cache-line isolation.
- `arc::Fanin` gives per-producer SPSC lanes when producer preemption must not block completed work from other producers.
- `arc::SeqReg` gives multi-word latest-snapshot handoff without queues or torn reads.
- `arc::dmabuf`, `arc::simdbuf`, `arc::ahbbuf`, `arc::axibuf`, and one-word STL allocators make DMA/SIMD/descriptor/RTC-capable heap placement explicit.
- `arc::Space` reports runtime flash, OTA slot, partition, and heap capacity without heap allocation.
- `arc::Ota` wraps staged OTA writes and slot state without raw handle plumbing.
- `arc::Store` gives typed NVS blobs and fixed-buffer strings without raw handle plumbing in user code.
- `arc::net::Radio` is the shared Core 0 Wi-Fi owner, so UDP and ESP-NOW do not each re-create NVS, netif, event loop, and Wi-Fi driver state.
- `arc::net::Eap` configures WPA2/WPA3 Enterprise credentials and joins through the shared radio without pulling WPA supplicant into non-enterprise builds.
- `arc::net::Tcp` gives direct TCP socket clients for Core 0 control/config paths.
- `arc::net::Tls` gives direct ESP-TLS client sessions for secure caller-buffer transports such as MQTTS without taking over reconnect or protocol policy.
- `arc::net::Http` gives RAII ownership for ESP-IDF HTTP/HTTPS client sessions, with explicit HTTPS factories when secure scheme enforcement matters.
- `arc::net::Mqtt` gives caller-buffer MQTT 3.1.1 packet encoding, parsing, and topic matching without owning the transport lane.
- `arc::net::Ws` gives WebSocket handshake helpers plus caller-buffer frame encode/parse without owning reconnect or task policy.
- `arc::net::Coap` gives caller-buffer CoAP datagram encode/parse and option walking without hiding message layout.
- `arc::net::Mdns` adds a thin discovery battery on top of `esp_netif` and the shared Wi-Fi lane when lwIP mDNS headers exist.
- `arc::Ble` gives a NimBLE lifecycle, host-task, GAP, advertising, and scanning bridge without taking over GATT profile design.
- `arc::net::Udp` is a reusable Core 0 transport plane when you opt into `#include "arc/udp.hpp"`.
- `arc::net::EspNow` is a reusable Core 0 raw-radio plane when you opt into `#include "arc/espnow.hpp"`.

The umbrella `arc.hpp` only surfaces optional batteries like `Mdns` when the matching SDK headers are visible through `__has_include(...)`; Arc does not fake protocol availability.

## Deliberate Non-Goals

Arc stays at the deterministic hardware, memory, transport, and wire-codec boundary. It does not try to be Arduino, Matter, RainMaker, LVGL, TinyUSB, AWS IoT, Azure IoT, or a portable HAL for unrelated chips.

When those stacks are needed, keep them on Core 0 and compose them with Arc's typed buffers, transport lanes, and ownership boundaries. Arc's job is to make the silicon-facing substrate hard to misuse, not to own product policy.

## Framework Comparison

Arc is closest to raw ESP-IDF, not Arduino. It deliberately keeps ESP-IDF's driver model, target specificity, and `esp_err_t` failure surface, then adds typed ownership, topology claims, memory-placement types, and Core 0/Core 1 architecture on top.

| Stack | Best at | Trade-off vs Arc |
| --- | --- | --- |
| Raw ESP-IDF | Maximum upstream coverage, newest Espressif APIs, official driver behavior, direct Kconfig/CMake control. | App code must manually police handle lifetime, pin conflicts, DMA-capable memory, cache ownership, task placement, and protocol buffer ownership. |
| Arc on ESP-IDF | Deterministic ESP32-S3 firmware where topology, memory capabilities, DMA/cache handoff, task placement, and lock-free lanes should be explicit in types. | Intentionally S3-locked, stricter, less beginner-friendly, and not a batteries-included product framework. |
| Arduino-ESP32 | Fast sketches, broad library ecosystem, simple GPIO/Wi-Fi demos, and lower onboarding cost. Espressif's Arduino-ESP32 documentation currently tracks the 3.x core on ESP-IDF 5.5-era bases. | Less direct control over scheduler, memory placement, driver lifetime, CMake graph, and zero-allocation realtime boundaries. |
| PlatformIO ESP-IDF / Arduino | Multi-board project management and convenient package workflows around ESP-IDF or Arduino projects. | Another build/package abstraction layer sits above the real ESP-IDF/Arduino stack; deterministic firmware still has to solve the same ownership and timing problems. |
| ESPHome / product DSL stacks | Configuration-driven devices, sensors, home automation integrations, OTA/user features. | Excellent product glue, but not intended for tight Core 1 loops, explicit DMA/cache protocols, or compile-time hardware topology proofs. |

Use raw ESP-IDF when the app needs full upstream surface area more than Arc's type discipline. Use Arduino when iteration speed and library availability matter more than deterministic ownership. Use Arc when a board has a fixed ESP32-S3 topology and the cost of a hidden allocation, cache bug, task migration, or peripheral double-init is unacceptable.

<details>
<summary>Feature Coverage Matrix</summary>

Legend: `native` means the stack has a first-class surface for that area; `manual` means possible but app code owns the dangerous details; `external` means bring another stack/library; `limited` means partial or board/package dependent.

| Area | Arc | Raw ESP-IDF | Arduino-ESP32 | ESPHome / product DSL |
| --- | --- | --- | --- | --- |
| Core/task topology | native typed static tasks, planes, `Tight` loops | native FreeRTOS primitives, manual policy | simplified task access, app usually sketch-shaped | generated runtime |
| ESP32-S3 targeting | strict S3 contract | configurable across Espressif targets | board package selection | board package selection |
| Pin/topology conflicts | native `Pins` and `Claim` tokens | manual discipline | manual discipline | generated YAML validation |
| Dedicated GPIO hot path | native typed `Drive`/`Sense` | LL/HAL possible manually | limited/manual | no |
| DMA/cache ownership | native `Cache`, DMA tickets, coherent helpers | available but manual | mostly hidden or library-specific | no |
| Capability heap buffers | native RAII `CapsBuf`, `dmabuf`, `simdbuf`, descriptor buffers | `heap_caps_*` manual | limited/manual | no |
| Lock-free lanes | native `Spsc`, `Mpsc`, `DenseMpsc`, `Fanin` | manual/external | external | no |
| Latest snapshot handoff | native `Reg`, dual-slot `SeqReg` | manual/external | external | no |
| Static pinned tasks | native `TaskMem`, `Plane`, `App`, `Ble::host` | native FreeRTOS manual | possible/manual | generated |
| SPI/I2C/UART/I2S | native typed owners | native drivers | friendly wrappers | components |
| SPI/I2C slave | native ownership and claims | native drivers | limited/manual | limited/no |
| ADC oneshot/continuous DMA | native oneshot, calibration, DMA scope | native drivers | simplified | sensor components |
| LCD I80/RGB/DVP DMA | native typed DMA ownership | native drivers | libraries/manual | limited/external |
| MCPWM/LEDC/RMT/PCNT/SDM | native typed wrappers | native drivers | mixed wrappers/libraries | components/limited |
| TWAI/CAN | native typed owner | native driver | library/manual | component-limited |
| USB Serial/JTAG | native typed byte IO | native driver | friendly wrapper | limited/no |
| USB OTG PHY | native low-level PHY owner | native/private-ish | TinyUSB-facing paths | no |
| Filesystem/File/SD/NVS/OTA | native RAII wrappers | native drivers | friendly wrappers | product-native |
| Wi-Fi owner | native shared Core 0 radio | native APIs | friendly wrappers | product-native |
| WPA Enterprise | native EAP bridge | native APIs | available/manual | limited |
| ESP-NOW/UDP/TCP/TLS/HTTP | native lanes/transports | native/lwIP/esp-tls/http | friendly wrappers | components |
| MQTT/WebSocket/CoAP | native no-heap codecs, caller-owned transport | mqtt/http libs or manual codecs | libraries | components |
| BLE host/GAP | native NimBLE lifecycle bridge | native Bluedroid/NimBLE | friendly wrappers | components |
| AES/SHA/HMAC/DS/MPI/XTS | native typed crypto/security wrappers | native/mbedTLS APIs | available/manual | limited |
| ULP/RTC GPIO/Sleep/PM/WDT | native typed wrappers where S3 support is verified | native APIs | mixed/manual | sleep components |
| Touch/temp/fuse/rng/space telemetry | native typed wrappers | native APIs | mixed/manual | components |
| Cloud/Matter/LVGL/TinyUSB class stacks | external by design | external/native components | libraries | product-native |
| Cross-target portability | no, S3 only | high within ESP-IDF | high within Arduino core | high within DSL |
| Beginner velocity | low | medium | high | very high |
| Realtime determinism | high by design | possible with discipline | low/medium | low/medium |

</details>

Arc benchmark policy is strict:

- Host benchmarks compare Arc primitives/codecs against local standard-library baselines only where both sides are compiled in the same binary.
- `tools/ensure-frameworks.sh` creates local ignored framework checkouts beside `esp-idf/`, including `arduino-esp32/`.
- `tools/framework-compare.sh` prints the feature matrix, runs the Arc host benchmark, reports local raw ESP-IDF and Arduino-ESP32 versions, and compiles a real Arduino `Print.cpp` host comparison where that source is host-buildable.
- Raw ESP-IDF benchmarks must live as pinned ESP-IDF examples or components and report firmware size, build config, target, and measured hardware path.
- Arduino or PlatformIO benchmarks must pin the core/platform version and board package in CI before any number is published.
- No README number should compare against Arduino, ESPHome, PlatformIO, or raw ESP-IDF unless that exact competitor source is checked in or installed by CI and run in the same workflow.

Reference docs: [ESP-IDF ESP32-S3 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/), [Arduino-ESP32 documentation](https://docs.espressif.com/projects/arduino-esp32/), [Arduino as an ESP-IDF component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html), [PlatformIO ESP-IDF framework docs](https://docs.platformio.org/en/latest/frameworks/espidf.html), and [ESPHome ESP32 platform docs](https://esphome.io/components/esp32.html).

<details>
<summary>Quick API Map</summary>

| Area | Headers | Primary types |
| --- | --- | --- |
| Core plane | `arc/task.hpp`, `arc/plane.hpp`, `arc/sketch.hpp`, `arc/tight.hpp` | `arc::spawn`, `arc::Plane`, `arc::App`, `arc::Tight` |
| Ownership and topology | `arc/topology.hpp`, `arc/claim.hpp`, `arc/init.hpp`, `arc/audit.hpp` | `arc::Pins`, `arc::Topology`, `arc::Claim`, `arc::Gate`, `arc::TryGate`, `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, `arc::RefLease`, `arc::Audit` |
| Memory and coherency | `arc/caps.hpp`, `arc/cache.hpp`, `arc/copy.hpp`, `arc/place.hpp` | `arc::dmabuf`, `arc::simdbuf`, `arc::Cache`, `arc::Copy` |
| Lock-free lanes | `arc/spsc.hpp`, `arc/mpsc.hpp`, `arc/fanin.hpp`, `arc/reg.hpp`, `arc/seq.hpp` | `arc::Spsc`, `arc::Mpsc`, `arc::DenseMpsc`, `arc::Fanin`, `arc::Reg`, `arc::SeqReg` |
| GPIO and timing | `arc/drive.hpp`, `arc/sense.hpp`, `arc/gpio.hpp`, `arc/rtc.hpp`, `arc/timer.hpp`, `arc/clock.hpp`, `arc/probe.hpp` | `arc::Drive`, `arc::Sense`, `arc::Gpio`, `arc::RtcGpio`, `arc::RtcPin`, `arc::Timer`, `arc::Clock`, `arc::Probe` |
| Buses and data plane | `arc/i2c.hpp`, `arc/spi.hpp`, `arc/i2s.hpp`, `arc/uart.hpp`, `arc/usb.hpp`, `arc/i80.hpp`, `arc/dvp.hpp` | `arc::I2cBus`, `arc::SpiBus`, `arc::I2s`, `arc::Uart`, `arc::Usb`, `arc::I80`, `arc::Dvp` |
| Storage and update | `arc/fs.hpp`, `arc/file.hpp`, `arc/sd.hpp`, `arc/store.hpp`, `arc/ota.hpp`, `arc/space.hpp` | `arc::Fs`, `arc::File`, `arc::Sd`, `arc::Store`, `arc::Ota`, `arc::Space` |
| Network and radio | `arc/net.hpp`, `arc/udp.hpp`, `arc/espnow.hpp`, `arc/tcp.hpp`, `arc/tls.hpp`, `arc/http.hpp`, `arc/mqtt.hpp`, `arc/ws.hpp`, `arc/coap.hpp`, `arc/mdns.hpp`, `arc/eap.hpp` | `arc::net::Radio`, `arc::net::Udp`, `arc::net::EspNow`, `arc::net::Tcp`, `arc::net::Tls`, `arc::net::Http`, `arc::net::Mqtt`, `arc::net::Ws`, `arc::net::Coap`, `arc::net::Mdns`, `arc::net::Eap` |
| Stream utilities | `arc/stream.hpp` | `arc::net::Stream`, `arc::net::ByteStream` |
| Security and silicon | `arc/aes.hpp`, `arc/sha.hpp`, `arc/hmac.hpp`, `arc/sign.hpp`, `arc/mpi.hpp`, `arc/xts.hpp`, `arc/fuse.hpp`, `arc/rng.hpp` | `arc::Aes`, `arc::Gcm`, `arc::Sha`, `arc::Hmac`, `arc::Sign`, `arc::Mpi`, `arc::Xts`, `arc::Fuse`, `arc::Rng` |

</details>

## Layout

```text
.
├── CMakeLists.txt
├── cmake
│   ├── arc-deps.cmake
│   └── arc-idf.cmake
├── env.fish
├── env.sh
├── examples
│   ├── bridge
│   │   └── README.md
│   ├── dsp
│   │   └── README.md
│   ├── copy
│   │   └── README.md
│   ├── can
│   │   └── README.md
│   ├── dvp
│   │   └── README.md
│   ├── i2s
│   │   └── README.md
│   ├── i2c
│   │   └── README.md
│   ├── i80
│   │   └── README.md
│   ├── scope
│   │   └── README.md
│   ├── spi
│   │   └── README.md
│   ├── space
│   │   └── README.md
│   ├── count
│   │   └── README.md
│   ├── trace
│   │   └── README.md
│   ├── ota
│   │   └── README.md
│   ├── pulse
│   │   └── README.md
│   ├── probe
│   │   └── README.md
│   ├── pwm
│   │   └── README.md
│   ├── sigma
│   │   └── README.md
│   ├── sleep
│   │   └── README.md
│   ├── timer
│   │   └── README.md
│   ├── uart
│   │   └── README.md
│   ├── espnow
│   │   └── README.md
│   ├── store
│   │   └── README.md
│   ├── temp
│   │   └── README.md
│   └── udp
│       └── README.md
├── README.md
├── partitions_16mb.csv
├── sdkconfig.defaults
├── sdkconfig.defaults.release
├── tools
│   ├── check-repo.sh
│   └── clean-generated.sh
├── components
│   └── arc
│       ├── CMakeLists.txt
│       └── include
│           ├── arc.hpp
│           └── arc
│               ├── adc.hpp
│               ├── aes.hpp
│               ├── audit.hpp
│               ├── ble.hpp
│               ├── bridge.hpp
│               ├── bus.hpp
│               ├── cache.hpp
│               ├── can.hpp
│               ├── burst.hpp
│               ├── capture.hpp
│               ├── caps.hpp
│               ├── claim.hpp
│               ├── cfg.hpp
│               ├── clock.hpp
│               ├── count.hpp
│               ├── copy.hpp
│               ├── dsp.hpp
│               ├── drive.hpp
│               ├── dvp.hpp
│               ├── eap.hpp
│               ├── detail
│               │   ├── cold.hpp
│               │   └── owner.hpp
│               ├── fanin.hpp
│               ├── fence.hpp
│               ├── file.hpp
│               ├── fs.hpp
│               ├── fuse.hpp
│               ├── gpio.hpp
│               ├── hmac.hpp
│               ├── http.hpp
│               ├── coap.hpp
│               ├── mqtt.hpp
│               ├── espnow.hpp
│               ├── i2c.hpp
│               ├── i2c_slave.hpp
│               ├── i80.hpp
│               ├── i2s.hpp
│               ├── mpsc.hpp
│               ├── mdns.hpp
│               ├── otg.hpp
│               ├── plane.hpp
│               ├── ota.hpp
│               ├── probe.hpp
│               ├── pulse.hpp
│               ├── pwm.hpp
│               ├── reg.hpp
│               ├── result.hpp
│               ├── scope.hpp
│               ├── sd.hpp
│               ├── sense.hpp
│               ├── sha.hpp
│               ├── sign.hpp
│               ├── sleep.hpp
│               ├── sketch.hpp
│               ├── sigma.hpp
│               ├── spi.hpp
│               ├── spi_slave.hpp
│               ├── space.hpp
│               ├── spsc.hpp
│               ├── store.hpp
│               ├── task.hpp
│               ├── temp.hpp
│               ├── time.hpp
│               ├── touch.hpp
│               ├── tcp.hpp
│               ├── timer.hpp
│               ├── topology.hpp
│               ├── trace.hpp
│               ├── trax.hpp
│               ├── uart.hpp
│               ├── udp.hpp
│               ├── ulp.hpp
│               ├── usb.hpp
│               ├── wdt.hpp
│               ├── ws.hpp
│               ├── xts.hpp
│               └── wave.hpp
└── main
    ├── app_main.cpp
    ├── CMakeLists.txt
    └── Kconfig.projbuild
```

## What Is Tuned

- `-O2` via `CONFIG_COMPILER_OPTIMIZATION_PERF`
- CPU default frequency forced to `240 MHz`
- `gnu++26`
- C++ exceptions and RTTI disabled
- Xtensa-safe `-mtext-section-literals` for C++ units that host IRAM code
- `16 MB` flash header and partition layout for `ESP32-S3 N16R8`
- `8 MB` Octal PSRAM at `80 MHz`, exposed explicitly through caps allocation
- Explicit cache sync helpers for DMA/PSRAM buffers
- Hardware TWAI/CAN node with self-test loopback, ISR RX, and lock-free handoff
- Static FreeRTOS task allocation for the realtime plane
- Core 1 idle watchdog detached so a non-yielding loop can own that core
- Dedicated GPIO output and input on the hot path
- Hardware pulse counting through PCNT
- Hardware symbol streaming through RMT
- Hardware symbol capture through RMT RX
- Hardware MCPWM waveform generation with runtime frequency and duty retuning
- Hardware complementary MCPWM pairs with explicit dead-time and brake/fault shutdown
- Hardware MCPWM edge capture for period/high/low measurement
- Hardware ADC streaming through the digital controller and DMA
- Hardware async memory copy through the ESP32-S3 DMA memcpy path
- Hardware LCD_CAM DVP camera input through DMA-backed frame buffers
- Hardware LCD_CAM Intel 8080 parallel output through DMA-backed panel IO
- Hardware I2C master/slave bus/device binding for sensors, EEPROMs, displays, control chips, and peer controllers
- Hardware SPI master/slave transfers with explicit bus/device composition and DMA-capable paths
- Hardware I2S streaming with duplex DMA and event counters
- Hardware UART serial lanes for GPS, modems, consoles, and legacy links
- Hardware USB Serial/JTAG byte IO for cabled control channels
- Hardware USB OTG PHY ownership for native host/device stack bring-up
- Hardware SD/MMC FAT mounting and raw sector access
- Hardware PWM offload through LEDC for periodic output that should not burn a core
- Hardware Sigma-Delta pulse-density output with IRAM-safe density updates enabled
- Hardware timebase and alarms through GPTimer
- Deep-sleep and light-sleep entry with explicit wake-source and power-domain policy
- Hardware-backed AES, GCM, SHA, HMAC, Digital Signature, and XTS flash-encryption paths through Espressif's crypto drivers
- ULP RISC-V and FSM load/run/control hooks for low-power coprocessor work
- Hardware die-temperature telemetry for thermal guard logic
- Hardware capacitive touch sensing with typed controller/channel ownership, filter hooks, and sleep-wakeup hooks
- Lock-free SPSC/MPSC queues and single-word control register
- Slot-aware OTA helper for staged writes and rollback state
- Typed NVS persistence and bounded string loading on the Core 0 side
- SPIFFS and FAT-on-flash VFS mounting
- RAII file handles for mounted VFS/FAT/SPIFFS/LittleFS paths
- Direct TCP clients for small control/config protocols
- RAII HTTP client sessions for REST/config exchanges
- BLE 5 host lifecycle, GAP naming, advertising, and scanning through NimBLE
- Bounded MPSC fan-in for multi-task telemetry producers
- SIMD-friendly math kernels that fit the Core 1 compute plane
- Cycle-counter instrumentation for hot-path measurement

## C++ Reality

The current local Arc toolchain was checked against the installed ESP-IDF environment on this machine:

- ESP-IDF `v6.0-dirty`
- `xtensa-esp32s3-elf-g++ 15.2.0`
- Espressif toolchain `esp-15.2.0_20251204`
- `__cplusplus == 202400L`
- libstdc++ `__GLIBCXX__ == 20250808`

Useful features present for Arc:

- `consteval`, `constinit`, concepts, `if consteval`, C++26 constexpr upgrades
- pack indexing and placeholder variables on the compiler side
- `std::expected`, `std::span 202311`, `std::byteswap`, `std::to_underlying`, `std::unreachable`

Features intentionally not treated as Arc foundation:

- standard `<simd>`, `<mdspan>`, `<inplace_vector>`, and `<hive>` are not available in this target libstdc++ build
- `<experimental/simd>` exists, but Arc keeps DSP kernels explicit and allocation-controlled instead of binding the framework to an experimental ABI
- `<print>` exists, but firmware logging stays on ESP-IDF logging paths

## Granular Builds

Arc no longer needs to drag every hardware driver into every app.

- `components/arc` now exports only the shared core headers and dependencies.
- `arc.hpp` only pulls feature headers when the matching ESP-IDF headers are visible through the current target's `REQUIRES`.
- `cmake/arc-deps.cmake` gives a terse way to declare only the Arc features a target actually uses.

Use this pattern in `main/CMakeLists.txt` or any example `main/CMakeLists.txt`:

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core gptimer)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

Feature names map directly to hardware lanes:

- `gpio`
- `adc`
- `aes`
- `ble`
- `copy`
- `eap`
- `fuse`
- `mcpwm`
- `rmt`
- `mpi`
- `pcnt`
- `sha`
- `sign`
- `ledc`
- `gptimer`
- `hmac`
- `http`
- `mdns`
- `i2c`
- `i2s`
- `spi`
- `sd`
- `twai`
- `cam`
- `lcd`
- `sdm`
- `temp`
- `store`
- `net`
- `ota`
- `otg`
- `pm`
- `rng`
- `rtc`
- `space`
- `time`
- `trax`
- `fs`
- `tcp`
- `tls`
- `udp`
- `uart`
- `ulp`
- `usb`
- `wdt`
- `xts`
- `espnow`

Add `gpio` when you use `arc::Drive`, `arc::Sense`, or raw `arc::Gpio`, because those pin APIs depend on the GPIO driver headers.
Add `rtc` when you use `arc::RtcGpio` or `arc::RtcPin`; it maps to the public RTC IO driver and should be used for sleep-retained pad state, pad isolation, and RTC wake plumbing.
Add `i2c` for both master (`arc::I2cBus`, `arc::I2c`) and slave (`arc::I2cSlave`) ownership; they share the same ESP-IDF I2C driver component and the same Arc port claim lane.
Add `spi` for both master (`arc::SpiBus`, `arc::Spi`) and slave (`arc::SpiSlave`) ownership; they share the same ESP-IDF SPI driver component and the same Arc host claim lane.

For the fastest compile times, keep each app honest: include only the Arc headers you use directly, and request only the matching Arc features in CMake.

## Programming Model

Arc should feel like a small framework, not like a bag of headers.

- The user-facing program goes in `main/app_main.cpp`.
- `app::boot()` is the normal signature entry you expose for your program.
- `extern "C" void app_main()` stays as the single ESP-IDF boundary.
- For a trivial app, `app::boot()` may be the only thing `app_main()` does.
- For a richer app, `main/app_main.cpp` still shows the program topology directly.
- Define one type alias per physical peripheral in one central place; do not instantiate the same GPIO, bus, timer, or channel with different template aliases across multiple files.

The shipped baseline is a tiny follow program:

- `arc::Sense` samples the configured input pin.
- `arc::Drive` drives the configured LED pin.
- Core 1 mirrors input level to output level with no queue, no heap, and no RTOS API in the hot loop.

`main/app_main.cpp` looks like this:

```cpp
#include "arc.hpp"

using Button = arc::Sense<arc::cfg::sense, 0>;
using Led = arc::Drive<arc::cfg::led, 0>;

struct Follow {
    static void setup()
    {
        Button::in();
        Led::out();
        Led::off();
    }

    IRAM_ATTR static void loop() noexcept
    {
        if (Button::high()) {
            Led::on();
        } else {
            Led::off();
        }
    }
};

using Core1 = arc::App<Follow, arc::cfg::stack, arc::Core::core1>;

namespace app {
inline void boot() { Core1::boot("arc"); }
}

extern "C" void app_main()
{
    app::boot();
}
```

The core clue is explicit where it matters:

- `arc::Core::core1` in `arc::App<...>` means the hot loop is pinned to Core 1.
- `arc::net::Udp` always owns Core 0 when you use it.

## Hardware Topology

C++ cannot globally prove that two unrelated translation units did not instantiate the same physical pin through two different template aliases. Arc handles this with a strict topology pattern: declare the board truth once, then import aliases from that one place.

```cpp
struct Board {
    using pins = arc::Pins<4, 5, 12, 13>;

    using Led = arc::Drive<4, 0>;
    using Button = arc::Sense<5, 1>;
    using Spi = arc::SpiBus<SPI2_HOST, 12, 13>;
};

static_assert(arc::Topology<Board>);
```

`arc::Pins` ignores negative sentinel pins like `-1`, so optional CS/MISO-style values can still be represented without false collisions.

Peripheral wrappers also claim physical silicon at `init()` time. Two different `arc::Uart`, `arc::I2cBus`, `arc::I2cSlave`, `arc::I2c`, `arc::SpiBus`, `arc::SpiSlave`, or CS-backed `arc::Spi` template aliases cannot silently own the same hardware with different type parameters; the later claim returns `ESP_ERR_INVALID_STATE`.
Identical aliases share a tiny atomic init gate, so two tasks racing the same first `init()` do not double-call the ESP-IDF allocator.

## Resource Lifetime

Arc keeps driver lifetime explicit because ESP-IDF peripheral handles often have process-wide side effects.

- `arc::Init` is the smallest boot-once state machine: `empty -> busy -> ready`, with `take()` giving exclusive teardown access. `arc::InitTxn` wraps first initialization so early returns roll back to `empty` instead of leaving a resource stuck in `busy`.
- `arc::RefInit` is the shared-resource variant. The first `begin()` owns initialization, later `begin()` or `take()` calls acquire a reference, and only the final `drop()` returns `true` so the caller can tear the hardware down. `arc::RefInitTxn` gives the first initializer the same rollback-on-early-return protection as `arc::InitTxn`.
- Both init machines expose `is_empty()`, `is_busy()`, and `is_ready()` so diagnostics can inspect state without depending on sentinel values.
- `arc::RefLease` is the scoped form for borrowed shared references. `take()` acquires a new reference, `adopt()` wraps an existing one, and the final owner must call `release()` and perform teardown explicitly instead of hiding hardware deinit in a destructor.
- `arc::Gate` is a tiny task-context guard for protecting short control-plane mutations. It asserts in ISR context instead of blocking where FreeRTOS cannot safely sleep.
- `arc::TryGate` is the non-blocking form for paths that may probe ownership but must not sleep; failed acquisition is just `false`.

This keeps static peripheral wrappers cheap while still giving buses, radios, filesystems, and host tasks a path to safe dynamic off/on behavior when an application needs sleep, hot-plug, or staged subsystem startup.

## License

Arc is licensed under `AGPL-3.0-only`, with the license notice in `LICENSE` and the full GNU Affero GPL v3 text in `COPYING`.

The intent is strict open source: you can study, use, modify, and redistribute Arc under AGPLv3-only, but downstream modifications must preserve the same reciprocal source-availability obligations.

### Dependency Compatibility

Arc depends on ESP-IDF components selected through `cmake/arc-deps.cmake`, including FreeRTOS, lwIP, mbedTLS, NimBLE/Bluetooth, FATFS/SPIFFS, and Espressif driver components. Those upstream components are generally permissive or Apache-style licenses, which are compatible with combining into an AGPLv3-only Arc firmware tree.

The practical consequence is not permissive: Arc-derived code remains AGPLv3-only. If a product cannot satisfy AGPL source-availability and reciprocal licensing obligations for its Arc-derived portions, the license is the blocker, not the build system.

## Start From Zero

### 1. Install prerequisites

You need at minimum:

- `git`
- `python3`
- a working serial permission setup for your board

ESP-IDF will install its own toolchain and Python environment with `install.sh`.

### 2. Create your own project from the template

Web:

- Open `https://github.com/dhimasardinata/arc`
- Click `Use this template`

CLI:

```bash
gh repo create my-fw --template dhimasardinata/arc --private
git clone https://github.com/<you>/my-fw.git
cd my-fw
```

If you just want to inspect Arc directly:

```bash
git clone https://github.com/dhimasardinata/arc.git
cd arc
```

## Install ESP-IDF

Arc supports both global and project-local ESP-IDF.

### Option A: global ESP-IDF

This is the better default if you work on more than one ESP-IDF repo.

```bash
mkdir -p ~/esp
cd ~/esp
git clone --branch v6.0 --recursive https://github.com/espressif/esp-idf.git
./esp-idf/install.sh esp32s3
```

Then expose it in your shell:

```bash
export IDF_PATH="$HOME/esp/esp-idf"
```

You can put that line in `.bashrc` or `.zshrc`.

### Option B: project-local ESP-IDF

Use this if you want the toolchain and IDF checkout pinned next to the repo.

```bash
git clone --branch v6.0 --recursive https://github.com/espressif/esp-idf.git esp-idf
./esp-idf/install.sh esp32s3
```

## Load the environment

From the project root:

```bash
. ./env.sh
```

For fish:

```fish
source ./env.fish
```

The env loader works in this order:

- `ARC_IDF_PATH`
- existing global `IDF_PATH`
- local `./esp-idf`

It also exports `IDF_TARGET=esp32s3`, and every project CMake entry forces `esp32s3` again during configure. Arc is intentionally single-target.

## First Build

Build the default dev profile:

```bash
idf.py build
```

Those defaults already assume `ESP32-S3 N16R8`. There is no secondary PSRAM overlay anymore because PSRAM is part of the baseline target.

Flash and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Release Profile

Arc ships a separate release overlay in `sdkconfig.defaults.release`.

It is more aggressive than the default profile:

- app logs off
- bootloader logs off
- runtime log level control off
- tag-level cache/list off
- `esp_err_to_name()` lookup tables off
- silent assertions/check macros
- silent reboot panic handler

If you are switching from the default profile to release in the same `build/` directory, clean first:

```bash
idf.py fullclean
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.release" build
```

If you want to keep dev and release artifacts side by side, use a second explicit build directory:

```bash
idf.py -B build-release -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.release" build
```

Release:

```bash
idf.py fullclean
idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.release" build
```

## Command Reference

Build:

```bash
idf.py build
```

Size summary:

```bash
idf.py size
```

Per-component size:

```bash
idf.py size-components
```

Per-file size:

```bash
idf.py size-files
```

Build and size summary:

```bash
idf.py build size
```

Build and per-component size:

```bash
idf.py build size-components
```

Build and per-file size:

```bash
idf.py build size-files
```

Build, size, and upload:

```bash
idf.py -p /dev/ttyACM0 build size flash
```

Build, size, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build size flash monitor
```

Build, per-file size, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build size-files flash monitor
```

Flash:

```bash
idf.py -p /dev/ttyACM0 flash
```

Erase flash:

```bash
idf.py -p /dev/ttyACM0 erase-flash
```

Decode the built partition table:

```bash
python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin
```

Dump source/docs into one clean folder:

```bash
python tools/dump-source.py
```

The dump output is the flat folder `dump/files`. It only contains files, not nested folders. Original paths are encoded into filenames with `__`, and `MANIFEST.txt` maps dump filenames back to source paths. It only copies C/C++ sources, headers, README files, CMake/text files, and `partitions*.csv`; it skips shell scripts, generated builds, ESP-IDF, `.espressif`, and all `sdkconfig*` files.

Monitor:

```bash
idf.py -p /dev/ttyACM0 monitor
```

Build and upload:

```bash
idf.py -p /dev/ttyACM0 build flash
```

Upload and monitor:

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

Build, upload, and monitor:

```bash
idf.py -p /dev/ttyACM0 build flash monitor
```

Clean generated local state across root and examples:

```bash
./tools/clean-generated.sh
```

## API

### `arc::Soc`

Compile-time ESP32-S3 capability map.

- Uses ESP-IDF `soc_caps.h` directly, so the constants match the installed target headers.
- Exposes feature booleans such as `wifi`, `ble`, `ble5`, `ble_mesh`, `ble_privacy`, `usb_otg`, `usb_serial_jtag`, `etm`, `simd`, `async_memcpy`, `ahb_gdma`, `dedicated_gpio`, `rtc_gpio`, `rtc_gpio_io`, `rtc_gpio_hold`, `rtc_gpio_wake`, `lcd_i80`, `lcdcam_dvp`, `aes_dma`, `sha512_256`, `hmac`, `sign`, `ecdsa`, `flash_xts`, `ulp_fsm`, and `ulp_riscv`.
- Exposes hardware counts such as `gpio_pins`, `rtc_gpio_pins`, `adc_units`, `ledc_channels`, `spi_peripherals`, `rmt_words`, `uart_ports`, `sdmmc_slots`, `rsa_bits`, and `ds_bits`.
- `etm` and `ecdsa` are deliberately represented even when the pinned ESP32-S3 `soc_caps.h` does not advertise those driver surfaces.
- Contains hard `static_assert` guards for Arc's baseline contract: dual-core ESP32-S3, dedicated GPIO, async AHB-GDMA, and SIMD.

### `arc::Pins<...>` and `arc::Topology<Board>`

Compile-time board topology guard.

- `arc::Pins<...>` asserts that all non-negative physical pins in the pack are unique.
- `arc::Topology<Board>` checks that `Board::pins` exists and passes the uniqueness rule.
- Use it in one central `Board`/`Hw` declaration, not scattered through application files.

This does not attempt impossible cross-translation-unit magic. It makes the intended hardware truth explicit and catches duplicate pins inside that truth source.

### `arc::Result<T>` and `arc::Status`

Opt-in C++23/26 runtime error surface based on `std::expected`.

- `arc::Result<T>` is `std::expected<T, esp_err_t>`.
- `arc::Status` is `std::expected<void, esp_err_t>`.
- `arc::fail(err)` creates the error side.
- `arc::status(err)` converts an `esp_err_t` into `arc::Status`.

Use this for runtime operations that can fail without invalidating the board topology. Hardware boot/configuration errors still intentionally use fail-fast ESP-IDF checks.

### `arc::Time`

Global microsecond clock backed by the ESP32-S3 SYSTIMER through `esp_timer_get_time()`.

- `us()` returns microseconds since boot.
- `ms()` returns milliseconds since boot.
- `next()` returns the nearest active esp-timer alarm timestamp.
- `wake()` returns the nearest alarm that should wake light sleep.
- `since(start)` and `due(start, span)` keep timeout math unsigned and terse.

Use this when Core 0 and Core 1 need one shared timestamp base. Keep `arc::Clock` for core-local cycle probes and short spin windows.

### `arc::Sha`

One-shot SHA hashing through Espressif's accelerated PSA/mbedTLS path.

- `Sha::sum<Type>(data, bytes)` returns a fixed-size `std::array` through `arc::Result`.
- `Sha::sum<Type>(span)` hashes typed contiguous storage using `span::size_bytes()`.
- `Sha::hash(...)` is the terse SHA-256 default.
- `Sha::bytes(type)` and `Sha::alg(type)` expose the digest size and PSA algorithm mapping.

Use this for telemetry digests, firmware metadata, and signed-payload staging without pulling raw PSA calls into app code.

### `arc::Aes` and `arc::Gcm`

AES contexts with explicit key setup and caller-owned buffers.

- `Aes::set(key, bytes)` accepts 128/192/256-bit keys and returns `esp_err_t`.
- `Aes::block(Aes::Way::enc/dec, in, out)` handles one 16-byte block.
- `Aes::cbc(...)`, `ctr(...)`, and `ofb(...)` expose the hardware-backed streaming modes without allocating.
- `Gcm::set(key, bytes)`, `seal(...)`, and `open(...)` cover authenticated encryption with caller-owned IV, AAD, output, and tag storage.

Use this when the data plane needs encryption/authentication before handing frames to Core 0 networking.

### `arc::Hmac`

eFuse-keyed SHA-256 HMAC through the ESP32-S3 HMAC peripheral.

- `Hmac::sum(key, data, bytes)` returns a 32-byte `arc::Hmac::Sum` through `arc::Result`.
- `Hmac::sum(key, span)` hashes typed contiguous storage using `span::size_bytes()`.
- `Hmac::jtag(key, token)` requests a temporary JTAG unlock with a 32-byte downstream token.
- `Hmac::off()` invalidates the temporary JTAG result.

Use this when secrets should stay in eFuse and only derived HMAC output should reach firmware.

### `arc::Sign`

Digital Signature peripheral surface for eFuse-key-backed RSA operations.

- `Sign::sign(message, data, key, signature)` performs the blocking raw DS signature operation.
- `Sign::start(...)` returns a move-only `Sign::Job` for split start/finish flows; `Job::finish()` releases the hardware lock, and the destructor finishes as a last-resort cleanup path.
- `Sign::busy()` reports the DS peripheral state.
- `Sign::seal(out, iv, plain, key)` encrypts plaintext DS parameters into the flash-storable `Sign::Data` format.
- `Sign::bytes(data)` returns the exact prepared-message and signature size for the encrypted RSA key.

Use this for signed telemetry, challenge responses, and provisioning flows where the private RSA material should never be readable by the main cores.

### `arc::Mpi`

Move-only owner for mbedTLS multiple-precision integers.

- `Mpi::from_be(bytes)` and `Mpi::from_le(bytes)` import caller-owned big-endian or little-endian buffers.
- `write_be(out)` and `write_le(out)` export into caller-owned storage.
- `bytes()`, `bits()`, `compare(...)`, `native()`, and `clone()` expose shape and interop without hidden allocation policy.
- `add(...)`, `sub(...)`, `mul(...)`, `mod(...)`, `mul_mod(...)`, `exp_mod(...)`, and `inv_mod(...)` return `arc::MpiResult<arc::Mpi>` carrying raw mbedTLS error codes.
- `Mpi::lock()` returns an RAII guard for direct ROM/register users of the RSA accelerator; normal `arc::Mpi` operations do not need it because the mbedTLS/ESP-IDF port locks internally.

Use this when firmware needs raw RSA-sized arithmetic, modular exponentiation, challenge math, or verification helpers without dropping into unmanaged `mbedtls_mpi` lifetime code.

### `arc::Xts`

XTS flash-encryption helpers for ESP32-S3 encrypted flash paths.

- `read(address, out, bytes)` calls `esp_flash_read_encrypted(...)` on the main flash chip.
- `write(address, data, bytes)` calls `esp_flash_write_encrypted(...)` and rejects unaligned encrypted writes before entering the driver.
- span overloads keep typed contiguous storage explicit without byte-count out parameters.
- `encrypted(partition)` exposes the partition-table encryption flag.
- partition `read(...)` and `write(...)` call `esp_partition_read/write(...)`; encrypted partition writes get the same 16-byte alignment guard before the ESP-IDF call.

Use this for encrypted data partitions, provisioning records, or secure boot metadata that must go through the hardware flash-encryption path instead of raw flash reads.

### `arc::Ulp`

ULP RISC-V and legacy FSM control surface.

- `load(binary)` copies a ULP binary into RTC memory.
- `run(wake)` configures and starts the ULP, while `start()` starts a previously configured binary.
- `stop()`, `resume()`, `halt()`, and `reset()` map directly to the ULP timer/core controls.
- `isr(handler, arg, mask)` and `off(handler, arg, mask)` manage main-core interrupt hooks from the ULP.
- `Ulp::Fsm::load(addr, binary)` loads a legacy FSM binary, and `Ulp::Fsm::macro(addr, program, words)` resolves macro labels before loading instruction arrays.
- `Ulp::Fsm::period(index, us)`, `run(entry)`, `stop()`, `resume()`, `isr(...)`, and `off(...)` expose the ESP32-S3 FSM timer and wake hooks directly.
- `Ulp::Word` is a 32-bit acquire/release shared word intended for RTC RAM placement.

Use this for always-on sensing, wake decisions, or low-power counters while the main cores sleep.

### `arc::Fuse`

eFuse and MAC read helpers.

- `bits(field)`, `bit(field)`, `read(field, data, bits)`, and `read<T>(field)` cover generated eFuse fields.
- `count(field)` reads popcount-style eFuse counters.
- `word(block, reg)` and `block(block, data, offset, bits)` expose raw block reads.
- `mac(type)`, `factory()`, and `custom()` return six-byte MACs through `arc::Result`.
- `pkg()`, `secure()`, `empty(block)`, `code(block)`, and `check()` expose common chip state.

Use this for identity, SKU, batch, secure-version, and provisioning reads without scattering raw eFuse headers through app code.

### `arc::Wdt`

Explicit Task Watchdog Timer surface.

- `init(timeout_ms, idle_cores, panic)` configures TWDT and treats an already initialized TWDT as ready.
- `tune(...)` reconfigures an active TWDT.
- `watch(task)`, `unwatch(task)`, `feed()`, and `check(task)` cover task subscription.
- `user(name)` returns a move-only RAII watchdog user; `User::feed()` feeds that user and `User::off()` unsubscribes it.
- `off()` deinitializes TWDT when no subscribed tasks/users remain.

Use this for loops that must prove forward progress independently of normal scheduler health.

### Failure Policy

Arc keeps two hardware-init surfaces on purpose:

- `init()`, `begin()`, `enable()`, `disable()`, and `set(...)` return `esp_err_t` when a caller can retry, degrade, or report the fault.
- `boot()`, `start()`, `stop()`, `on()`, and compile-time convenience helpers are fail-fast wrappers for hardware that is considered a board invariant.
- Data-plane methods that return `esp_err_t` or `arc::Result<T>` use the recoverable path internally. They do not hide an `ESP_ERROR_CHECK()` in their lazy initialization path.
- Raw `native()` accessors remain fail-fast because they hand out driver handles whose absence means the declared board topology is not usable.

Use fail-fast helpers in the fixed board bring-up path. Use the recoverable names in industrial boot code, optional peripherals, hot-plug style probes, or telemetry paths where a peripheral can be retried after power sequencing settles.

### Topology vs Runtime Values

Pins, peripheral instances, DMA sizing, queue depths, ISR affinity, and fixed bus shape stay as template parameters because changing them creates different hardware ownership. Runtime controls stay runtime:

- `arc::Uart::baud(value)` retunes a live UART without creating a second UART type.
- `arc::Uart::baud()` reports the live UART rate, while the template `Baud` stays the declared default.
- `arc::Spi::send/recv/xfer/poll(..., hz)` can override the clock for one transfer.
- `arc::Pwm::start(config)` and `arc::Pwm::hz(value)` let LEDC keep compile-time pin/channel ownership while the live waveform config comes from runtime data.
- `arc::Pwm::duty(value)` and `arc::Pwm::set(value)` update LEDC duty without instantiating another template.
- `arc::Sigma::set(value)` updates SDM density without instantiating another template.
- MCPWM wrappers expose runtime `hz(value)` and `duty(value)` where the hardware supports clean retuning.

Do not create multiple aliases for the same physical peripheral just to vary a runtime knob. Keep one topology type and drive live values through the runtime methods.

### Static Concepts

`arc/concepts.hpp` adds small compile-time contracts for app-side composition without virtual dispatch or heap-owned interfaces.

- `arc::WaveConfig<T>` checks that a static waveform type exposes `Config`, `defaults()`, `config()`, and `permille()`.
- `arc::ConfigWave<T>` checks the combined recoverable runtime-config surface.
- `arc::DutyWave<T>` checks the duty-control surface.
- `arc::RateWave<T>` checks the runtime frequency-control surface.

Use these when application code should accept "anything with this static control surface" instead of naming one exact Arc wrapper type.

### Internal Contracts

Most application code should only see the small surface: `start()`, `send(...)`, `read(...)`, `set(...)`, and typed buffers. The dense internals are deliberately isolated:

- Claim tokens protect hardware ownership across translation units.
- Cache helpers encode ESP32-S3 DMA/cache-line rules at the call site.
- Lock-free queues use explicit sequence arithmetic and cache-line isolation.
- ISR-facing callbacks stay minimal and update counters or queues with release/acquire ordering.

Only extend those internals when you are also preserving their hardware contract. New application features should normally compose the existing lanes instead.

### `arc::Drive<Pin, Channel, Core>`

Dedicated GPIO output bound at compile time.

- `out()` routes the pin to dedicated output.
- `on()` and `off()` are the clearest hot-path writes.
- `hi()` and `lo()` remain available as low-level synonyms.
- `write<true>()` and `write<false>()` are zero-cost compile-time forms.

`arc::Dio` remains available as a compatibility alias.

### `arc::Sense<Pin, Channel, Core>`

Dedicated GPIO input bound at compile time.

- `in()` routes the pin to dedicated input.
- `high()` and `low()` read the dedicated input register directly.

`arc::Din` remains available as a compatibility alias.

### `arc::App<Program, StackBytes, Core>`

The smallest way to run a program on one core.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void loop() noexcept`

`App` turns that into a static pinned task without heap allocation.

`boot()` is idempotent while the task is active.

`arc::Sketch` remains available as a compatibility alias.

### `arc::Tight<Program, StackBytes, Core = core1, Guard = arc::Critical, Pri = max, Budget = 0>`

Masked per-step loop for the extreme path.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void step() noexcept`

`Tight` runs `step()` forever and constructs `Guard` around every iteration.

When `Budget` is non-zero, `Tight` counts iterations whose masked step exceeds that cycle budget. `overruns()` returns the counter, and an optional `Program::overrun(cycles) noexcept` hook can consume the exact overrun cost.

Use this only when the step body is tiny and the jitter budget is tighter than what a normal task iteration should tolerate. `arc::Hard` is a compatibility alias.

### `arc::Link<Event, Control, Capacity>`

Shared state for asymmetric programs.

- `events` is an `arc::Spsc<Event, Capacity>`
- `pace` is an `arc::Reg<std::uint32_t>`
- `control` is an `arc::Reg<Control>`

Use it when Core 1 emits ordered events and Core 0 applies latest-wins control.

`arc::Bus` remains available as a compatibility alias.

### `arc::Spsc<T, Capacity>`

Bounded lock-free lane for one producer and one consumer.

- `try_push(event)` is producer-only.
- `try_pop(event)` is consumer-only.
- `drain(scratch, fn, max)` batches consumer work without heap allocation.
- `cap()` exposes the usable capacity; the backing ring keeps one slot empty.
- `bytes()` exposes the queue object footprint.
- Payloads stay trivially copyable and the backing ring size remains a power of two.

Use this when event history matters and the ownership contract is exactly one writer and one reader. `arc::Ring<T, Capacity>` is the compatibility alias for the same type.

`arc::Audit<arc::Spsc<T, Capacity>>` keeps the same API but binds the first producer and consumer it sees and fires `configASSERT` if another task/thread touches that endpoint later.

### `arc::Mpsc<T, Capacity>`

Bounded lock-free fan-in for many producers and one consumer.

- `try_push(event)` can be called by multiple producer tasks.
- `try_pop(event)` is single-consumer and fits Core 0 drain loops.
- `drain(scratch, fn, max)` batches consumer work without heap allocation.
- `cap()` exposes the power-of-two static capacity.
- `cell_align()`, `cell_bytes()`, and `bytes()` expose the queue's RAM geometry.
- Sequence checks use explicit 32-bit modular deltas, so wrap is handled on the queue clock instead of pointer-width signed math.
- Payloads stay trivially copyable and capacity remains a power of two.
- Each cell is cache-line isolated to avoid producer/consumer false sharing, so large capacities intentionally spend more internal RAM than a packed queue.
- CAS contention uses a tiny capped `nop` backoff in IRAM instead of immediately hammering the shared head cache line.

Use this when several OS-side tasks with the same scheduling priority need to feed one telemetry or transport owner without a FreeRTOS queue. If producer preemption must never block completed work from another producer, use `arc::Fanin`.

`arc::DenseMpsc<T, Capacity>` keeps the same queue semantics but drops cache-line isolation and packs each cell to the payload's natural alignment. Use it when internal RAM density matters more than worst-case false-sharing avoidance. `arc::Audit<arc::Mpsc<T, Capacity>>` adds a fail-fast single-consumer assertion on top of the cache-line-isolated lane, and `arc::Audit<arc::DenseMpsc<T, Capacity>>` combines both trade-offs.

### `arc::Mux<T, Capacity>`

Compatibility alias for `arc::Mpsc<T, Capacity>`.

Use it when the code should read as a topology: many inputs into one owner. It is declared in `arc/mpsc.hpp`; prefer `arc::Mpsc` when the concurrency contract should be visible at the type site.

### `arc::Fanin<T, Capacity, Producers>`

Static fan-in made from one SPSC lane per producer and one round-robin consumer.

- `try_push<Producer>(event)` is wait-free for that producer lane.
- `try_pop(event)` drains any completed producer without waiting behind another producer's half-finished slot.
- `try_pop(producer, event)` also reports which lane produced the event.
- `drain(scratch, fn, max)` accepts either `fn(event)` or `fn(producer, event)`.
- `cap()` is the usable per-producer lane capacity, `producers()` is the static lane count, and `bytes()` exposes the full object footprint.

Use this when producer identity is static and tail latency matters more than one global FIFO order.

`arc::Audit<arc::Fanin<T, Capacity, Producers>>` keeps the same API and asserts that each lane remains single-producer and the fan-in side stays single-consumer.

### `arc::Pwm<Pin, Hz, DutyPermille = 500, Channel = 0, Timer = Channel % 4, Bits = 10>`

Compile-time hardware PWM on ESP32-S3 LEDC.

- `begin()` configures timer, channel, and pin routing and returns `esp_err_t`.
- `begin(config)` applies caller-provided runtime frequency and duty while keeping the pin/channel topology fixed in the type.
- `start()` configures timer, channel, pin routing, and starts output.
- `start(config)` is the fail-fast runtime-config path when frequency or boot duty comes from NVS, Kconfig, or provisioning.
- `hz()` and `permille()` report the live configured values.
- `config()` returns the live runtime PWM configuration snapshot.
- `hz(value)` retunes the live PWM frequency through the recoverable path.
- `set(config)` reapplies both runtime PWM knobs through the recoverable path.
- `on()` reapplies the current duty instead of the compile-time default.
- `off()` stops output low.
- `pause()` and `resume()` gate the underlying LEDC timer.
- `duty<permille>()` updates duty with a compile-time value.
- `duty(permille)` and `set(permille)` update duty with a runtime value.
- `frequency()` and `duty_permille()` remain the compile-time defaults for code that wants the declared board configuration.

Use this when the waveform is periodic and the silicon should generate it. Keep `arc::Wave` for cases where the CPU must own every edge.

### `arc::Sigma<Pin, SampleHz = 1'000'000, Initial = 0>`

Compile-time Sigma-Delta Modulator output.

- `init()` creates the SDM channel and applies the initial density as a recoverable operation.
- `enable()` and `disable()` gate the channel and return `esp_err_t`.
- `start()` creates and enables one SDM channel routed to the pin.
- `write(value)` updates pulse density in the hardware driver.
- `write<Value>()` range-checks the density at compile time.
- `set(value)` is the recoverable runtime density update path.
- `zero()`, `high()`, and `low()` are shorthand targets.
- `fast(value)` is the thinnest post-boot path and returns `esp_err_t` instead of panicking.

Use this when you want hardware pulse-density output or a low-pass-filtered analog-ish voltage without burning CPU cycles on a PWM loop.

### `arc::Pulse<Pin, Hz, DutyPermille = 500, Group = 0, ResolutionHz = 10'000'000>`

Compile-time MCPWM waveform wrapper.

- `start()` allocates the timer, operator, comparator, and generator, then starts the hardware waveform.
- `start(config)` keeps the MCPWM ownership in the type while letting the boot frequency/duty come from runtime data.
- `hz()` and `permille()` report the live waveform state, while `config()` snapshots both together.
- `hz(value)` retunes the timer period at runtime.
- `duty(value)` retunes the duty cycle at runtime.
- `set(config)` and `set(permille)` expose recoverable retune paths for app code that should not hard-panic on bad runtime values.
- `on()` and `off()` force a level without tearing down the hardware path.
- `wave()` returns control to the timer/comparator path after a forced level.
- `frequency()` and `duty_permille()` remain the declared compile-time defaults.

Use this when LEDC is too small a hammer and you want a stronger waveform block with explicit period and compare control.

### `arc::Bridge<HighPin, LowPin, Hz, DutyPermille = 500, RiseDelayTicks = 0, FallDelayTicks = 0, ...>`

Compile-time MCPWM complementary pair wrapper.

- `start()` allocates one timer, one operator, one comparator, and two generators.
- `start(config)` keeps the bridge topology static while the boot frequency/duty can come from runtime data.
- `hz()` and `permille()` report the live switching state, while `config()` snapshots the mutable runtime knobs.
- `hz(value)` retunes the switching frequency at runtime.
- `duty(value)` retunes the base duty cycle at runtime.
- `set(config)` and `set(permille)` expose recoverable retune paths without reallocating the hardware lane.
- `wave()` restores complementary switching after a forced state.
- `hi()`, `lo()`, and `off()` force safe states onto the pair without deleting the hardware path.
- optional trailing fault parameters bind one GPIO fault to the MCPWM brake path in hardware.
- `recover()` asks the operator to leave a one-shot brake state after the external fault clears.
- `frequency()`, `duty_permille()`, `rise_delay_ticks()`, and `fall_delay_ticks()` keep the declared compile-time defaults available to board code.

Use this when the output is no longer “just PWM” and you need a half-bridge style pair with explicit dead-time and a CPU-independent shutdown path.

### `arc::Capture<Pin, Hz = 1'000'000, Group = 0, Prescale = 1, Rise = true, Fall = true>`

Compile-time MCPWM capture wrapper.

- `start()` allocates a capture timer and channel and starts timestamping selected edges.
- `ticks()` returns the latest captured timestamp.
- `period()`, `high()`, and `low()` expose the measured edge deltas with no queue allocation.
- `rising()` tells you which edge arrived last.
- `soft()` triggers a software capture for bring-up and inspection.

Use this when you want real edge timestamps without a CPU spin loop or GPIO ISR plumbing.

### `arc::Adc<Io, Atten = ADC_ATTEN_DB_12, Width = SOC_ADC_DIGI_MAX_BITWIDTH>`

Compile-time ADC pad descriptor for `arc::Scope` and `arc::AdcOne`.

- `io()` returns the GPIO number.
- `atten()` returns the attenuation used for the digital pattern.
- `width()` returns the configured ADC bit width.
- `bitwidth()` returns the oneshot driver bit-width enum.

Use this as a pad specifier, not as a runtime object.

### `arc::AdcBus<Unit = ADC_UNIT_1>` and `arc::AdcOne<Bus, Pad, Cali = true>`

Compile-time ADC oneshot wrapper with optional eFuse calibration.

- `AdcBus::init()` owns one ADC oneshot unit handle.
- `AdcBus::off()` deletes that unit handle when oneshot users are done.
- `AdcOne::init()` configures one ADC channel on the explicit bus.
- `raw()` returns a recoverable `arc::Result<int>` raw conversion.
- `mv()` returns calibrated millivolts through the ESP-IDF calibration scheme.
- `off()` releases the channel claim and calibration handle.

Use this for battery dividers, thermistors, slow sensors, and board health reads where a DMA stream would be wasteful.

### `arc::Scope<Hz, FrameBytes = 256, StoreBytes = 1024, Flush = true, Pads...>`

Compile-time ADC continuous wrapper.

- `start()` boots the ADC digital controller and DMA path.
- `pull(...)` reads and parses samples into `adc_continuous_data_t` in one call.
- `raw(...)` exposes the raw DMA frame if you want to own parsing yourself.
- `frames()` and `overruns()` expose ISR-side frame and overflow counters.
- `flush()` drops queued samples and resets the software counters.

Use this when analog throughput matters more than per-sample polling.

### `arc::SpiBus<Host, Sclk, Mosi, Miso = -1, MaxBytes = 4096, ...>`

Compile-time SPI bus wrapper.

- `init()` initializes one ESP32-S3 SPI host and returns `esp_err_t`.
- `boot()` initializes one ESP32-S3 SPI host with DMA-capable transfer sizing.
- `off()` frees the SPI host when no devices are still mounted.
- `host()`, `sclk()`, `mosi()`, `miso()`, `quadwp()`, and `quadhd()` keep the routing obvious in user code.
- optional trailing bus pins enable the native quad WP/HD and octal data4..data7 routes without changing existing aliases.
- `Spi::dual`, `Spi::quad`, and `Spi::octal` are transaction flags for multiline data transfers.
- host claims reject incompatible second aliases for the same physical SPI block.
- The SPI ISR is pinned to CPU0 by default so transport work naturally stays off the realtime core.

Use this when multiple devices should share one bus or when bus routing should stay explicit and reusable.

### `arc::Spi<Bus, Cs = -1, Hz = SPI_MASTER_FREQ_20M, Mode = 0, Queue = 4, ...>`

Compile-time SPI device wrapper.

- `init()` adds one device and returns `esp_err_t`.
- `boot()` adds one device on top of a compile-time `arc::SpiBus`.
- `off()` removes the device and releases its CS claim.
- `send(...)`, `recv(...)`, `xfer(...)`, and `poll(...)` cover the common synchronous paths.
- The synchronous transfer helpers accept an optional final `hz` argument for per-transfer clock overrides.
- `queue(...)` and `wait(...)` expose the interrupt-driven transaction path when you want multiple jobs in flight.
- `Move` and `StrictMove` are ticket objects that own queued transaction storage until `finish(...)`.
- `queue_coherent(move, tx, rx, bytes)` flushes TX, safely discards whole RX cache lines, and queues one DMA transfer.
- `finish_coherent(move)` waits for that exact SPI transfer and invalidates RX before the CPU reads it.
- Ticketed finishes may be called out of queue order; raw `wait(...)` still exposes the driver's FIFO completion stream.
- `acquire()` and `release()` give explicit bus ownership when CS must stay held.

Use this when bytes should move through the SPI engine and DMA path instead of a software bit loop.

### `arc::SpiSlave<Host, Sclk, Mosi, Miso, Cs, Queue = 4, Mode = 0, ...>`

Compile-time SPI slave wrapper.

- `init()` owns one SPI2/SPI3 host in slave mode and returns `esp_err_t`.
- `boot()` initializes the slave host and fail-fasts on impossible board topology.
- `enable()`, `disable()`, `start()`, and `stop()` gate bus participation without deleting the host.
- `send(...)`, `recv(...)`, and `xfer(...)` cover synchronous transactions initiated by an external master.
- `queue(...)` and `wait(...)` expose the interrupt-driven slave queue when the master clocks frames asynchronously.
- `Move` and `StrictMove` are ticket objects that keep queued descriptor storage valid until `finish(...)`.
- `queue_coherent(move, tx, rx, bytes)` flushes MISO data, discards whole MOSI RX cache lines, and queues one DMA transaction.
- `finish_coherent(move)` waits for that exact slave transaction and invalidates RX before the CPU reads it.
- The slave wrapper claims the same physical SPI host lane as `arc::SpiBus`, so master/slave aliases cannot silently collide.
- ESP32-S3 SPI slave DMA still depends on sane external CS timing; if a master hammers CS edges faster than the hardware can settle, diagnose the board timing before blaming the wrapper.

Use this when the ESP32-S3 should be a deterministic peripheral for another controller without leaving SPI ownership in raw C driver calls.

### `arc::I2cBus<Port, Sda, Scl, Pullup = true, ...>` and `arc::I2c<Bus, Addr, Hz = 400'000, ...>`

Compile-time I2C master wrapper.

- `I2cBus::init()` brings up one I2C master bus and returns `esp_err_t`.
- `I2cBus::probe(addr)` checks for a device without forcing a reboot loop.
- `I2cBus::off()` deletes the bus once devices are removed.
- `I2c::init()` mounts one device on a bus and returns `esp_err_t`.
- `I2c::off()` removes the device and releases its address claim.
- `send(...)`, `recv(...)`, and `xfer(...)` cover write, read, and repeated-start write/read transactions.
- `boot()` remains available when board topology failure should be fail-fast.

Use this for sensors, EEPROMs, small displays, PMICs, and control chips that should stay outside the Core 1 hot loop.

### `arc::I2cSlave<Port, Sda, Scl, Addr, TxBytes = 256, RxBytes = 256, ...>`

Compile-time I2C slave wrapper.

- `init()` creates one ESP32-S3 I2C slave device and returns `esp_err_t`.
- `boot()` creates the slave device and fail-fasts on impossible board topology.
- `listen(on_request, on_receive, user)` registers ISR-context callbacks without hiding that received bytes arrive through the driver callback buffer.
- `send(...)` writes bytes into the slave TX FIFO/ringbuffer for the external master to read and can report the actual accepted byte count.
- `reset()` clears pending TX data and resets the hardware TX FIFO.
- `off()` deletes the slave device and releases the I2C port claim.
- The slave wrapper claims the same physical I2C port lane as `arc::I2cBus`, so master/slave aliases cannot silently collide.

Use this when the ESP32-S3 should present a small register/status surface to another controller while keeping request/receive ISR hooks explicit.

### `arc::Can<Tx, Rx, Bitrate = 500'000, Queue = 8, RxDepth = 8, ...>`

Compile-time ESP32-S3 TWAI/CAN node wrapper.

- `init()` creates one on-chip TWAI node and binds ISR callbacks as a recoverable operation.
- `boot()` creates one on-chip TWAI node and binds ISR callbacks.
- `enable()` and `disable()` gate bus participation and return `esp_err_t`.
- `start()` and `stop()` enable or disable bus participation.
- `frame(id, payload, ext, remote)` builds an owning classic CAN frame.
- `send(frame, timeout_ms)` queues a frame; keep the frame alive until TX completes.
- `send_wait(frame, timeout_ms)` queues and waits for completion before returning.
- `recv(frame)` drains the ISR-backed lock-free `arc::Spsc` RX lane.
- `filter(...)`, `dual(...)`, and `range(...)` configure hardware acceptance filters while the node is disabled.
- `sent()`, `done()`, `rx()`, `drop()`, `err()`, `bytes()`, and `idle()` expose bus counters.

Use this for robot/industrial control links where the TWAI controller should own bit timing and arbitration, not software.

### `arc::I2s<Bclk, Ws, Dout = I2S_GPIO_UNUSED, Din = I2S_GPIO_UNUSED, Hz = 48'000, ...>`

Compile-time standard-mode I2S wrapper.

- `init()` allocates and initializes TX, RX, or duplex channels and returns `esp_err_t`.
- `boot()` allocates and initializes TX, RX, or duplex channels.
- `start()` and `stop()` gate the hardware stream without deleting the channels.
- `preload(...)` primes TX DMA before enabling the lane.
- `write(...)` and `read(...)` move frames through DMA-backed channels.
- `preload(span)`, `write(span)`, and `read(span)` also expose `arc::Result<std::size_t>` overloads for runtime failures that should stay recoverable.
- `sent()`, `recv()`, `send_ovf()`, and `recv_ovf()` expose ISR-side event counters.

Use this when framed serial audio or sample streams should be owned by the I2S block, not by a CPU copy loop.

### `arc::I2sTdm<Bclk, Ws, Dout = I2S_GPIO_UNUSED, Din = I2S_GPIO_UNUSED, Hz = 48'000, ...>`

Compile-time TDM-mode I2S wrapper.

- `init()` allocates and initializes TX, RX, or duplex TDM channels and returns `esp_err_t`.
- `boot()`, `start()`, `stop()`, `pause()`, and `resume()` follow the same lifecycle as `arc::I2s`.
- `preload(...)`, `write(...)`, and `read(...)` reuse the same DMA-backed byte/span surface as standard mode.
- `rate(value)` reconfigures the TDM clock while the lane is stopped, restoring the running state after retune.
- `mask()` returns the compile-time active slot mask, and `slots()` returns the effective total slot count.
- `tx_info()`, `rx_info()`, `sent()`, `recv()`, `send_ovf()`, and `recv_ovf()` expose runtime lane state and ISR-side counters.
- The wrapper auto-picks a safer MCLK multiple for wider multichannel frames on ESP32-S3 so common 4-slot / 32-bit layouts do not start from the warning path.

Use this for multichannel codecs, TDM ADCs, and framed sample fabrics where more than two slots must move through one DMA-backed serial lane.

### `arc::I2sPdm<Clk, Dout = I2S_GPIO_UNUSED, Din = I2S_GPIO_UNUSED, Hz = 16'000, ...>`

Compile-time one-line PDM-mode I2S wrapper for ESP32-S3.

- `init()` allocates and initializes TX, RX, or duplex PDM channels and returns `esp_err_t`.
- `boot()`, `start()`, `stop()`, `pause()`, and `resume()` follow the same lifecycle as the other Arc I2S lanes.
- `preload(...)`, `write(...)`, and `read(...)` reuse the same DMA-backed byte/span surface as standard mode and TDM.
- `rate(value)` reconfigures the PDM clock while the lane is stopped, restoring the running state after retune.
- `mask()` exposes the compile-time RX slot mask, and `data()` exposes whether software sees PCM or raw PDM words.
- `tx_info()`, `rx_info()`, `sent()`, `recv()`, `send_ovf()`, and `recv_ovf()` expose runtime lane state and ISR-side counters.
- The wrapper keeps the one-line codec-style path explicit and constrains ESP32-S3 PDM use to I2S0, which is what the hardware actually supports.

Use this for digital MEMS microphones, simple PDM speaker/codec links, and low-part-count audio/control planes that should stay on the DMA-backed I2S hardware path.

### `arc::Uart<Port, Tx, Rx, Rts = UART_PIN_NO_CHANGE, Cts = UART_PIN_NO_CHANGE, ...>`

Compile-time UART wrapper.

- `init()` configures the port, pins, framing, buffers, and driver, then returns `esp_err_t`.
- `boot()` keeps the fail-fast path for required serial links.
- `off()` deletes the driver and releases the port claim.
- `write(...)` and `read(...)` expose `arc::Result<std::size_t>` ergonomic overloads.
- `available(...)`, `wait(...)`, `flush()`, `baud()`, and `baud(value)` expose the common runtime controls.

Use this for GPS receivers, modem AT links, binary serial protocols, and board-level debug channels that should not leak raw UART driver calls into app code.

### `arc::Usb<Tx = 256, Rx = 256>`

Compile-time USB Serial/JTAG wrapper.

- `init()` installs the driver with fixed TX/RX ring sizes and returns `esp_err_t`.
- `boot()` keeps the fail-fast path for required cabled control.
- `write(...)` and `read(...)` expose `arc::Result<std::size_t>` byte-count overloads.
- `connected()`, `installed()`, `wait(...)`, and `off()` expose runtime state and teardown.

Use this for ESP32-S3 USB console/control traffic when the native USB Serial/JTAG peripheral is the lane.

### `arc::Otg`

RAII owner for the ESP32-S3 native USB OTG PHY.

- `host(...)` and `device(...)` install the internal or external PHY in the requested OTG mode.
- `make(config)` keeps the full `usb_phy_config_t` path available when board wiring needs every field.
- `mode(...)` switches the PHY OTG mode through ESP-IDF's USB PHY driver.
- `status(target)` reads whether a PHY target is free or in use.
- `off()` releases the PHY, and the destructor does the same for move-owned handles.

Use this as the low-level USB OTG lane owner before a host/device stack takes over the DWC core. ESP-IDF v6.0 in this repo does not ship a TinyUSB component, so Arc exposes the real PHY boundary instead of inventing a partial HID/MSC wrapper.

### `arc::Sd<Clk = 14, Cmd = 15, D0 = 2, D1 = 4, D2 = 12, D3 = 13, ...>`

Compile-time SD/MMC FAT wrapper.

- `mount(base)` mounts one SD/MMC card into VFS using the configured bus width and pins.
- `boot(base)` keeps the fail-fast path for required storage.
- `read(...)` and `write(...)` expose raw sector operations for preallocated buffers.
- `status()`, `format()`, `sector()`, `bytes()`, `card()`, and `unmount()` expose card control.

Use this for removable FAT storage and high-volume logs that should stay on Core 0.

### `arc::Dvp<arc::DvpLines<...>, Vsync, Pclk, De, Xclk>`

Compile-time DVP camera input using the ESP32-S3 LCD_CAM block.

- `arc::DvpLines<...>` declares 8, 10, 12, or 16 data GPIOs.
- `init()` creates one DVP controller with compile-time sync/data routing and returns `esp_err_t`.
- `boot()` creates one DVP controller with compile-time sync/data routing.
- `enable()` and `run()` expose recoverable controller enable/start steps.
- `start()`, `stop()`, `pause()`, and `resume()` gate the capture lane.
- `buffer<T>(count)` allocates a DMA-capable camera frame buffer.
- `grab(frame, &bytes, timeout_ms)` receives one frame into user-owned storage.
- `convert(in, out, ...)` exposes the hardware format-conversion hook.
- `frames()` and `bytes()` expose received-frame counters.

Use this for camera or parallel input streams that should land in RAM through LCD_CAM/DMA instead of CPU sampling.

### `arc::I80Bus<arc::Lines<...>, Dc, Wr>` and `arc::I80<Bus, Cs>`

Compile-time Intel 8080 parallel output using the ESP32-S3 LCD_CAM block.

- `arc::Lines<...>` declares 8 or 16 data GPIOs.
- `I80Bus::init()` creates one DMA-backed I80 bus and returns `esp_err_t`.
- `I80Bus::boot()` creates one DMA-backed I80 bus.
- `I80::init()` creates one panel IO endpoint and returns `esp_err_t`.
- `I80::boot()` creates one panel IO endpoint.
- `param(cmd, data, bytes)` sends command parameters.
- `color(cmd, data, bytes)` queues a DMA-backed payload transfer.
- `Ticket` captures the exact queued color transfer so `finish(ticket)` does not wait on unrelated later transfers.
- `color_coherent(ticket, cmd, buffer)` flushes the draw buffer before LCD_CAM owns it.
- `buffer<T>(count)` allocates a draw buffer with the alignment/caps expected by the I80 path.
- `sent()`, `done()`, `bytes()`, `idle()`, `ready(ticket)`, `wait()`, and `finish(ticket)` expose the DMA queue state.

Use this for display or parallel-device throughput that should be owned by LCD_CAM/DMA, not by a GPIO loop.

### `arc::Rgb<arc::RgbLines<...>, Hsync, Vsync, De, Pclk, Disp, ...>`

Compile-time RGB LCD output using the ESP32-S3 RGB panel path.

- `arc::RgbLines<...>` declares the RGB data bus width in 8-bit steps.
- `init()` / `boot()` create the panel, register ISR callbacks, then reset and initialize the panel handle.
- `init(fb0, fb1, fb2)` accepts user-owned static frame buffers, so the realtime plane does not need heap-backed screen memory.
- `off()` tears the panel down and releases the RGB hardware claim.
- `draw(...)`, `draw_2d(...)`, and `frame(...)` expose the ESP-LCD panel bitmap path directly.
- `draw_coherent(...)` and `frame_coherent(...)` flush cache before handing a draw buffer to the RGB path.
- `refresh()` exposes refresh-on-demand mode, while `restart()` exposes the RGB DMA resync hook.
- `pclk(value)`, `disp(on)`, `sleep(enable)`, `mirror(x, y)`, `swap(xy)`, `gap(x, y)`, `invert(enable)`, and `yuv(cfg)` expose the live panel controls.
- `fb(index)` returns the current driver- or user-owned frame buffer pointer, and `buffer<T>(count)` allocates an aligned draw buffer through the RGB driver.
- `draws()`, `frame_done()`, `vsyncs()`, and `pixels()` expose the ISR-side panel telemetry.
- 16-bit and 24-bit buses can rely on the ESP-LCD default color-format inference. 8-bit buses must pass an explicit input color format.

Use this for raw RGB TFT panels, LVGL/direct-framebuffer surfaces, and any display path that should stay on the S3 LCD DMA engine instead of a serialized bus.

### `arc::Copy<Backlog = 4, BurstBytes = 64, Weight = 0, Backend = arc::CopyBackend::auto_dma>`

Compile-time async DMA memcpy wrapper.

- `init()` installs one async memcpy driver instance and returns `esp_err_t`.
- `boot()` installs one async memcpy driver instance.
- `send(dst, src, bytes)` queues a non-blocking DMA copy and returns immediately after submission.
- `copy(dst, src, bytes)` submits the transfer and spins until the completion counter reaches the target.
- `send_coherent(ticket, dst, src, bytes)` flushes the source cache, discards destination cache lines, queues the DMA copy, and stores the exact completion target in `ticket`.
- `finish_coherent(ticket)` waits for that exact transfer and invalidates the destination cache before CPU reads it.
- `ready(ticket)` reports whether that exact transfer has completed.
- `copy_coherent(dst, src, bytes)` is the blocking one-call form built on the non-blocking ticket path.
- coherent copy destination buffers must cover whole cache lines; the `_strict` variants also require the source side to be cache-line aligned.
- Non-strict coherent copies are for explicitly shared cache-line handoffs. Do not mutate unrelated data sharing those boundary cache lines while DMA owns the destination.
- `sent()`, `done()`, `bytes()`, and `idle()` expose lock-free counters without FreeRTOS queues.
- `arc::CopyBackend::ahb` pins the backend to AHB-GDMA on ESP32-S3.

Use this when bytes should move through the DMA memcpy engine instead of burning CPU cycles on a software copy loop.

### `arc::Cache`

Explicit cache coherency helpers for DMA and external-memory paths.

- `to_device(data, bytes)` writes dirty cache lines back before hardware reads a buffer.
- `from_device(data, bytes)` invalidates only whole cache-line-aligned buffers after hardware writes a buffer.
- `discard(data, bytes)` writes back and invalidates only whole cache-line-aligned buffers when ownership moves away from the CPU.
- `from_device_unaligned(...)` and `discard_unaligned(...)` are the explicit escape hatches when the caller accepts shared-line invalidation risk.
- The unaligned escape hatches are unsafe around actively mutated neighbors on the same cache line; use cache-line-aligned buffers or the `_strict` path for live DMA ownership.
- `line(ptr)` returns the cache line size for one address, or zero when the address is not cacheable.
- span overloads work directly with `arc::CapsBuf<T>::view()`.

Use this when you are about to hand a buffer to DMA, SPI, I2S, ADC, RMT, or async copy and you want ownership to be explicit instead of implied.

### `arc::Burst<Pin, Hz, Symbols = 64, Depth = 1, Dma = false>`

Compile-time RMT TX wrapper for symbol streams.

- `start()` allocates and enables one TX channel plus a copy encoder.
- `symbol(d0, l0, d1, l1)` builds one `rmt_symbol_word_t` at compile time.
- `send(...)` pushes a symbol array and optionally hardware-loops it forever with `loop = -1`.
- `wait()` drains queued transfers.
- `carrier(...)` and `plain()` toggle carrier modulation.

Use this when you want deterministic pulse trains or protocol waveforms without a CPU spin loop.

### `arc::Trace<Pin, Hz, Symbols = 64, MinNs = 100, MaxNs = 200000, Dma = false>`

Compile-time RMT RX wrapper for symbol capture.

- `start()` allocates and enables one RX channel.
- `arm()` starts one receive job into a static symbol buffer.
- `ready()` reports whether one capture has completed.
- `view()` exposes the captured symbols as `std::span<const rmt_symbol_word_t>`.
- `size()`, `data()`, and `last()` expose the raw result without queueing or heap allocation.
- `carrier(...)` and `plain()` toggle RX-side carrier demodulation.

Use this when you need pulse widths, protocol symbols, or edge timing without CPU sampling.

### `arc::Trax`

Thin control lane for the ESP32-S3 Xtensa TRAX trace memory.

- `enable()` selects the current core's trace memory bank, while `enable(bank)` and `both(swap)` expose explicit S3 bank routing.
- `start(unit)`, `words()`, and `instr()` start capture with word or instruction downcount units.
- `active()` reports whether the current core is tracing.
- `stop(delay)` requests trace stop after the chosen post-trigger delay.
- `core(id)` maps core IDs to the corresponding TRAX bank selector.
- Calls return `ESP_ERR_NO_MEM` when the firmware was built without ESP32-S3 TRAX memory enabled.

Use this for internal execution-history capture. `arc::Trace` is RMT pin capture; `arc::Trax` is CPU trace memory.

### `arc::Count<EdgePin, ...>`

Compile-time PCNT wrapper for hardware edge accumulation.

- `start()` allocates the unit, configures the channel, enables the counter, and starts counting.
- `read()` returns the accumulated count.
- `clear()` zeroes the hardware counter.
- `watch(value)` and `unwatch(value)` manage PCNT watch points.
- `stop()` and `resume()` gate counting without deleting the unit.

Use this when the silicon should count pulses while the CPU only samples or reacts at a slower cadence.

### `arc::Timer<Hz, Source = GPTIMER_CLK_SRC_DEFAULT, Direction = GPTIMER_COUNT_UP>`

Compile-time GPTimer wrapper for a hardware timebase.

- `start()` boots and runs a free-running timer.
- `start<Handler>()` also binds a compile-time alarm ISR hook before enabling the timer.
- `alarm(count, reload, auto_reload)` arms the timer alarm.
- `read()` returns the current counter value.
- `zero()` sets the counter.
- `stop()`, `pause()`, and `resume()` gate the timer.
- Rebinding the same timer instance to a different `Handler` trips a debug assert instead of silently leaving the original ISR installed.

`Handler` supplies:

- `IRAM_ATTR static bool alarm(const gptimer_alarm_event_data_t&) noexcept`

Use this when you want a real hardware timebase or periodic alarm instead of a busy loop.

### `arc::Sleep`

Power-state helper for the Core 0 side.

- `after_us(value)` and `after<Value>()` arm timer wake.
- `ext0<Pin, High>()` arms single RTC-GPIO wake.
- `ext1<Mode, Pins...>()`, `ext1_off<Pins...>()`, and `ext1_status()` cover multi-pin RTC wake.
- `cause()` returns the previous wake source from standard ESP-IDF wakeup state.
- `woke(source)` tests one wake source.
- `keep(domain)`, `auto_power(domain)`, and `power_down(domain)` set sleep power-domain policy.
- `light()` enters light sleep and returns after wake.
- `deep()` enters deep sleep and does not return.

Use this when the most efficient loop is no loop at all.

### `arc::RtcGpio` and `arc::RtcPin<Pin>`

Verified RTC IO surface for sleep-domain pads.

- `RtcGpio::valid(pin)` and `RtcGpio::index(pin)` expose the ESP-IDF RTC IO mapping without guessing which GPIO numbers are RTC-capable.
- `RtcPin<Pin>::init()` claims one physical pad before routing it through RTC IO; `deinit()` releases it after routing back to normal IO MUX.
- `mode(...)`, `sleep_mode(...)`, `level()`, `write(...)`, `hi()`, and `lo()` cover explicit RTC input/output ownership.
- `pullup(...)`, `pulldown(...)`, `floating()`, `drive(...)`, `hold()`, `unhold()`, and `isolate()` cover deep-sleep leakage and pad retention work.
- `wake(...)` and `wake_off()` expose the RTC GPIO wake hooks directly when you need per-pad wake control outside `arc::Sleep`.

Use this when a pad must keep a deterministic state across light/deep sleep, when leakage must be cut before deep sleep, or when the RTC domain should own the pin instead of the digital GPIO matrix.

### `arc::Pm<Type, Arg = 0>`

Typed power-management lock wrapper.

- `init(name)` creates one ESP-IDF PM lock and returns `esp_err_t`.
- `acquire(name)` creates the lock if needed, then acquires it.
- `release()` releases one held lock count.
- `off()` deletes the lock after all active holds are released.
- `hold(name)` returns a move-only RAII guard.
- `arc::CpuLock`, `arc::ApbLock`, and `arc::SleepLock` are the normal one-word aliases.

Use this when dynamic frequency scaling or automatic light sleep is enabled and one critical path needs a stable CPU/APB/sleep contract.

### `arc::Rng`

Hardware random source wrapper.

- `word()` returns one 32-bit hardware random word.
- `fill(data, bytes)` fills a raw buffer.
- `fill(span)` fills fixed-extent or dynamic-extent mutable spans.
- `value<T>()` returns one trivially copyable value filled from hardware random bytes.

Use this for local nonces, fuzz seeds, randomized backoff, and binary IDs without plumbing raw ESP-IDF RNG calls through app code.

### `arc::Temp<Min = -20, Max = 100>`

Internal ESP32-S3 die-temperature helper.

- `init()` installs the temperature sensor and returns `esp_err_t`.
- `enable()` and `disable()` gate the sensor and return `esp_err_t`.
- `start()` installs and enables the temperature sensor once.
- `stop()` disables the sensor without deleting the driver object.
- `read(value)` writes Celsius into a `float&` and returns `esp_err_t`.
- `read()` returns Celsius through `arc::Result<float>`.
- `celsius()` returns Celsius and panics on driver error.
- `milli()` returns milli-Celsius for integer telemetry.

Use this as a Core 0 guard rail when radio, CPU, DMA, and waveform peripherals are running hard.

### `arc::TouchBus<...>` and `arc::Touch<Bus, Io, ...>`

Compile-time ESP32-S3 capacitive touch wrapper.

- `TouchBus::init()` creates the touch controller with one explicit sample configuration and returns `esp_err_t`.
- `enable()`, `disable()`, `start()`, `stop()`, and `scan(timeout_ms)` expose the controller lifecycle without hiding when the hardware is actually measuring.
- `filter(...)`, `events(...)`, `sleep(...)`, `waterproof(...)`, `proximity(...)`, and `denoise(...)` forward the ESP-IDF controller subfeatures through one typed owner.
- `Touch::init()` creates one touch channel bound to a compile-time GPIO/channel pair.
- `raw()`, `smooth()`, `benchmark()`, and `proximity()` expose the channel data plane as recoverable `arc::Result<std::uint32_t>` reads where the target supports that data type.
- `threshold(value, ...)` and `config(...)` let you retune the active threshold after an initial benchmark scan.
- `info()` exposes the resolved channel/GPIO metadata from the driver.
- `Touch::off()` deletes the channel; `TouchBus::off()` tears down the controller after channels are removed.

Use this for touch keys, wake pads, guarded control surfaces, and low-part-count HMI inputs that should stay typed and deterministic.

### `arc::Mask<Level = XCHAL_EXCM_LEVEL>`

Core-local Xtensa interrupt-level guard.

- construct it to raise the interrupt level for the current core
- destroy it to restore the previous level
- `arc::Critical` is the normal “silence OS-visible interrupts” alias
- `arc::Silence` raises all the way to level `15`

Use this only around tiny hot sections where determinism matters more than latency. The guard is force-inlined, so code using it inherits the caller's section; mark callers with `ARC_HOT` / `IRAM_ATTR` when they can run while flash cache is disabled.

### `arc::SeqReg<T>`

Seqlock-style latest-snapshot lane for payloads larger than one word.

- `write(value)` publishes one complete snapshot
- `write_unmasked(value)` is the raw cross-core-only fast path when the writer cannot be preempted by a same-core reader
- `read()` retries until one stable snapshot is observed
- `try_read(value)` gives you the same read without blocking

`SeqReg` is cache-line aligned to avoid false sharing with adjacent state. It now publishes into an inactive shadow slot before flipping the even sequence, so readers never copy from the slot a writer is mutating. `write()` still masks OS-visible interrupts around the odd sequence window, and `read()` inserts a tiny `arc::pause()` between failed snapshots so a fast writer does not turn the losing core into a wasteful full-bus spin.

Use this when `arc::Reg<T>` is too small but a queue would be wasteful.

### `arc::Probe` and `arc::CycleStats`

Cycle-counter instrumentation for hot paths.

- `arc::Probe::now()` snapshots the current Xtensa cycle counter.
- `probe.lap()` returns elapsed cycles since the snapshot.
- `arc::CycleStats::add(cycles)` tracks min/max/total/sample count with no heap.
- `arc::CycleStats::avg()` returns the integer average cycle count.

Use this when you want to validate the actual cost of a hot path instead of trusting intuition.

### `arc::dsp`

Hot-loop math helpers for the compute plane.

- `arc::dsp::dot(...)` computes one dot product.
- `arc::dsp::scale(...)` writes `out = in * gain`.
- `arc::dsp::mix(...)` writes `out = lhs + rhs`.
- `arc::dsp::mac(...)` accumulates `acc += in * gain`.
- `arc::dsp::peak(...)` finds the absolute peak.
- `arc::dsp::Fir<T, N>` provides a no-heap FIR state machine with `step(...)` and `run(...)`.

Use this when Core 1 is doing signal work and you want aligned buffers plus tight, vector-friendly loops without a heavyweight DSP framework.

### `arc::CapsBuf<T>` and explicit capability buffers

Typed heap slabs for memory placement that should stay obvious in user code.

- `arc::inbuf<T>(n)` allocates an internal RAM slab
- `arc::psbuf<T>(n)` allocates a PSRAM slab
- `arc::dmabuf<T>(n)` allocates a DMA-capable internal slab with cache-line alignment
- `arc::cachebuf<T>(n)` allocates a cache-line-capable internal slab
- `arc::simdbuf<T>(n)` allocates a SIMD-capable internal slab
- `arc::rtbuf<T>(n)` allocates an RTC RAM slab
- `arc::ahbbuf<T>(n)` allocates an AHB DMA descriptor slab
- `arc::axibuf<T>(n)` allocates an AXI DMA descriptor slab

Each returns `arc::CapsBuf<T>` with `data()`, `size()`, `bytes()`, and `view()`.

Standard containers can use the same placement rules through one-word allocators:

- `arc::RamAlloc<T>`
- `arc::PsramAlloc<T>`
- `arc::DmaAlloc<T>`
- `arc::SimdAlloc<T>`

Do not let a capability-allocated `std::vector` reallocate while DMA or a peripheral owns its `.data()` pointer. Reserve capacity before handoff, or prefer `arc::CapsBuf` / `std::array` for active hardware buffers.

### Placement aliases

Arc also exposes short placement aliases in `arc/place.hpp`:

- `ARC_HOT`, `ARC_FORCE_HOT`
- `ARC_TCM_CODE`, `ARC_TCM_DATA`
- `ARC_DRAM`, `ARC_DMA`, `ARC_DMA_ALIGN`
- `ARC_RTC`, `ARC_RTC_NOINIT`, `ARC_RTC_CODE`
- `ARC_EXT_BSS`, `ARC_EXT_NOINIT`, `ARC_NOINIT`

Use these when a variable or function must live in a specific memory region and you want that intent to stay visible in the source.

### `arc::Space`

Runtime capacity reporter.

- `flash(tag)` reports flash chip size, the running app slot, running OTA image state, the current image size, the percent used inside the active slot, the free bytes and free percent left in that slot, the percent used across all OTA app slots, and the boot plus next-update slots.
- `parts(tag)` reports every partition with address and size.
- `heap(tag)` reports 8-bit, internal, DMA, cache-aligned, SIMD, DMA-descriptor, IRAM-capable, RTC RAM, optional exec, and PSRAM heap capacity.
- `all(tag)` runs all three in one call.

Use this alongside `idf.py size`, `idf.py size-components`, and `idf.py size-files` when you want the real board view.

### `arc::Store`

Typed NVS blob and string storage for Core 0 work.

- `boot()` initializes NVS and repairs the partition if IDF reports version/free-page mismatch.
- `save(ns, key, value)` writes a trivially copyable value.
- `load(ns, key, value)` reads the exact typed blob back.
- `load_or(ns, key, fallback)` keeps user code short when missing data should fall back cleanly.
- `save_string(ns, key, value)` writes a NUL-terminated string.
- `string_size(ns, key, bytes)` reports the fixed buffer size needed to read a string, including the terminator.
- `load_string(ns, key, span, chars)` reads into caller-owned storage and reports characters excluding the terminator.
- `erase(ns, key)` removes one key and commits.

Use this when you want persistent config without hand-written handle lifetime code.

### `arc::File`

RAII file handle for mounted VFS paths.

- `open(path, mode)` returns `arc::Result<arc::File>`.
- `read(...)` and `write(...)` return byte counts through `arc::Result<std::size_t>`.
- `flush()`, `seek(...)`, `tell()`, and `close()` wrap the common `FILE*` operations.
- `native()` exposes the raw handle when an IDF API really needs it.

Use this after FAT, SPIFFS, LittleFS, or another ESP-IDF VFS mount is already active.

### `arc::Fs`

Mounted filesystem helpers for Core 0 storage paths.

- `spiffs(base, label, max_files, format)` mounts one SPIFFS partition.
- `spiffs_info(...)`, `spiffs_gc(...)`, `spiffs_check(...)`, and `spiffs_off(...)` cover common maintenance.
- `fat(base, label, max_files, format, alloc)` mounts FAT-on-flash through wear levelling.
- `fat_ro(...)`, `fat_info(...)`, `fat_format(...)`, `fat_off()`, and `fat_ro_off(...)` keep FAT control explicit.

Use this to create a mounted VFS path before using `arc::File`.

### `arc::Ota`

Typed OTA session wrapper for Core 0 work.

- `running()`, `boot()`, and `next()` expose the active, configured, and next update slots.
- `begin(session)` starts a staged write against the next OTA slot using sequential erase by default.
- `write(...)` and `write_at(...)` feed the slot.
- `finish(session)` validates the image and optionally activates it for next boot.
- `confirm()` marks the running image valid.
- `rollback()` forces rollback and reboot.

Use this when your transport plane needs to stage firmware without leaking raw OTA handles into app code.

### `arc::Ring<T, Capacity>`

Compatibility alias for `arc::Spsc<T, Capacity>`.

It is declared in `arc/spsc.hpp`; prefer `arc::Spsc` in new code when the concurrency contract should be visible at the type site.

### `arc::Reg<T>`

Single-word latest-wins lane.

- use for control words or pace values
- `T` must fit in 32 bits and be trivially copyable

### `arc::Plane<Work, StackBytes, State, Core>`

Pinned static task for a bound workload.

Your workload defines either:

- `setup()` and `run() noexcept`

or:

- `setup(state)` and `run(state) noexcept`

Use this when you want explicit stateful realtime workers instead of the simpler `Sketch`.

Bound workloads boot as `Plane<...>::boot<&shared>(tag)` so the shared cross-core state is named at compile time and cannot accidentally come from a temporary or stack object.

### `arc::net::Radio`

Shared Wi-Fi foundation for Core 0 transports.

- `base()` initializes typed NVS, `esp_netif`, the default event loop, the default STA netif, and the Wi-Fi driver exactly once.
- `prepare(mode, power_save)` sets storage, Wi-Fi mode, and power-save policy before a transport writes its own config.
- `start(mode, power_save)` starts Wi-Fi once and rejects later mode mismatches instead of silently reconfiguring a live radio.
- `sta()` returns the shared STA netif handle for transports that need IP state.
- `ap()` returns the shared SoftAP netif handle, creating the default AP netif only when AP mode is requested.
- `set(iface, config)` writes caller-owned `wifi_config_t` without constraining auth policy.
- `join(config, power_save, connect)` starts STA mode from a full `wifi_config_t`, so WPA3, OWE, PMF, roaming, and enterprise-prepared configs stay expressible.
- `ap(config, power_save)` starts SoftAP mode, and `apsta(ap_config, sta_config, power_save, connect)` starts combined AP+STA mode.
- `lease()` and `release()` let long-lived transports pin radio teardown while their event handlers are alive.
- `leases()` reports active transport pins.
- `stop()` stops Wi-Fi without throwing away the prepared configuration.
- `off()` deinitializes the Wi-Fi driver and destroys Arc-owned default STA/AP netif state only when no transport leases are active.

Long-lived transports release their lease only after their own `stop()` path has closed sockets or callbacks and exited the Core 0 task. `Radio::off()` returning `ESP_ERR_INVALID_STATE` means a transport is still alive, not that Wi-Fi teardown is broken.

Use `arc_requires(... net)` only when directly using `arc::net::Radio`. `udp` and `espnow` already include the same dependencies.

### `arc::net::Eap`

Opt-in WPA2/WPA3 Enterprise bridge for the shared STA radio.

- `identity(...)`, `user(...)`, `password(...)`, and `newpass(...)` configure PEAP/TTLS/EAP identity material.
- `ca(...)`, `cert(...)`, `bundle(...)`, `domain(...)`, `time(...)`, and `suiteb(...)` expose certificate validation and Suite-B controls.
- `methods(...)`, `phase2(...)`, `pac(...)`, `fast(...)`, and `okc(...)` expose the EAP method knobs without hiding ESP-IDF policy.
- `enable()`, `disable()`, and `clear()` map directly to station enterprise mode and credential lifetime.
- `join(config, power_save, connect)` applies the STA config, enables EAP, starts Wi-Fi, and optionally calls `esp_wifi_connect()`.

Use `arc_requires(... eap)` only when an app really needs enterprise authentication; normal `net`, `udp`, and `espnow` builds do not pull in WPA supplicant headers.

### `arc::net::Tcp`

Direct TCP client for Core 0 control and configuration paths.

- `dial(host, port, timeout_ms)` opens one socket and returns `arc::Result<arc::net::Tcp>`.
- `send(...)` and `recv(...)` return byte counts without hiding partial transfers.
- `recv(..., 0)` restores the POSIX-style blocking receive timeout without changing the send timeout.
- `send_all(...)` loops until the caller-owned buffer is fully written or an error occurs.
- `close()` and `native()` keep socket lifetime explicit.

Use this when a small protocol needs TCP but does not justify a framework-owned task.

### `arc::net::Tls`

Direct ESP-TLS client transport for Core 0 secure control and telemetry paths.

- `dial(host, port, cfg)` opens a blocking TLS session with caller-provided `esp_tls_cfg_t`.
- `send(...)` and `recv(...)` return byte counts without hiding partial transfers.
- `send_all(...)` loops until the caller-owned buffer is fully written or an error occurs.
- `avail()` exposes decrypted bytes already buffered by the TLS record layer.
- `close()` and `native()` keep TLS lifetime explicit.

Use this under `arc::net::Mqtt`, `arc::net::Ws`, or a custom stream protocol when you need TLS without adopting a cloud SDK, reconnect loop, or framework-owned task.

### `arc::net::Stream`

Tiny stream composition helpers for transports that expose `send_all(...)` and `recv(...)`.

- `ByteStream<T>` checks the minimal stream surface used by `arc::net::Tcp` and `arc::net::Tls`.
- `write(...)` forwards a caller-owned buffer to `send_all(...)`.
- `read_exact(...)` loops until a caller-owned buffer is full or the stream closes/errors.
- `write_frame16(...)` and `read_frame16(...)` encode/decode a two-byte big-endian length prefix around a caller-owned payload buffer.

Use this when an application protocol needs exact records on top of TCP or TLS but still should not own a reconnect loop, parser task, or heap buffer.

### `arc::net::Http`

RAII wrapper for ESP-IDF HTTP client sessions.

- `make(config)` returns `arc::Result<arc::net::Http>`.
- `https(config)` and `https(url, base)` require an `https://` URL before constructing the same RAII client.
- `url(...)`, `method(...)`, `header(...)`, and `body(...)` set request fields.
- `perform()` covers the simple one-shot request path.
- `open(...)`, `write(...)`, `fetch()`, `read(...)`, and `close()` expose the streaming path.
- `status()`, `length()`, and `native()` expose response metadata and the raw handle.

Use this for Core 0 REST/config exchanges where HTTP or HTTPS should stay outside the realtime plane. Use `arc::net::Tls` directly when you need a secure raw stream rather than HTTP semantics.

### `arc::net::Mqtt`

Thin MQTT 3.1.1 wire codec for caller-owned buffers.

- `connect(...)`, `publish(...)`, `subscribe(...)`, `ping(...)`, and `disconnect(...)` encode packets directly into a caller-provided byte span.
- `parse(...)` splits one MQTT frame out of a receive buffer without hiding the consumed byte count.
- `view(packet)`, `connack(packet)`, and `suback(packet)` decode the common packet bodies without heap allocation.
- `match(filter, topic)` applies MQTT wildcard rules so subscription routing can stay local and explicit.

Use this when you want MQTT batteries on top of `arc::net::Tcp` or another stream lane without giving the framework ownership of reconnect policy, socket lifetime, or tasking.

### `arc::net::Ws`

Thin WebSocket handshake and frame codec for caller-owned buffers.

- `key(out, nonce)` base64-encodes a caller-owned 16-byte nonce into a `Sec-WebSocket-Key`.
- `accept(out, key)` computes the RFC 6455 `Sec-WebSocket-Accept` response without heap allocation.
- `text(...)`, `binary(...)`, `ping(...)`, `pong(...)`, and `close(...)` encode one frame into a caller-provided byte span.
- `parse(frame, scratch)` decodes one frame, and unmasking stays explicit through the caller-provided scratch span.
- `close_view(frame)` decodes the close code and reason without hiding the wire payload.

Use this when you want WebSocket batteries on top of `arc::net::Tcp` or `arc::net::Http` without giving the framework ownership of handshake policy, reconnect loops, or tasking.

### `arc::net::Coap`

Thin CoAP datagram codec for caller-owned buffers.

- `message(...)` encodes one CoAP datagram from explicit type, code, token, options, and payload.
- `ping(...)` and `reset(...)` cover the empty-message path without retyping the header fields.
- `parse(...)` splits token, raw option bytes, and payload out of one received datagram.
- `next(options, offset, number)` walks decoded options in order without heap allocation or hidden iterators.
- `option(...)` and `text(...)` build explicit option descriptors for URI path/query and content-format composition.

Use this when you want CoAP on top of UDP or another datagram lane without hiding message IDs, token policy, retransmission policy, or blockwise state.

### `arc::net::Mdns`

Thin mDNS owner for the lwIP responder already underneath `esp_netif`.

- `host(name)` binds or renames the STA host advertisement on the shared `arc::net::Radio` interface.
- `ap(name)` does the same for the AP side when AP mode exists.
- `host(iface, name)`, `off(iface)`, `active(iface)`, `announce(iface)`, and `restart(iface)` expose the explicit per-netif path.
- `Service::make(...)` and `Service::ap(...)` return RAII service registrations that remove their slot on destruction.
- TXT generation stays an opt-in lwIP callback instead of a framework-owned string builder.
- `Mdns` uses `arc::Gate` internally, so keep it in task context; blocking from ISR or timer-callback contexts will assert instead of silently deadlocking.

Use this when you want discovery batteries without inventing a second Wi-Fi ownership model.

### `arc::Ble`

NimBLE host bridge for ESP32-S3 BLE.

- `init()` initializes NVS through `arc::Store` and starts the NimBLE controller/host stack.
- `run()` owns `nimble_port_run()` in the current task, while `host<Stack, Core, Priority>(name)` starts it in one static pinned task instead of using NimBLE's heap-created default task.
- Only one task can own the NimBLE run loop at a time.
- `stop(wait)`, `join(wait)`, and `active()` expose host-loop shutdown without racing `deinit()`, and the static host task is reaped by the caller after it unwinds instead of reusing task memory early.
- `deinit()` tears the stack down only after the host lane is inactive.
- `cfg()` exposes `ble_hs_cfg`, while `hook(reset, sync)` sets the common reset/sync callbacks.
- `gap()`, `name(...)`, and `appearance(...)` cover the standard GAP identity path.
- `own(...)` infers the address type, `addr(...)` copies the active address, and `fields(...)` / `reply(...)` install advertising and scan-response fields.
- `adv(...)`, `adv_stop()`, `scan(...)`, and `scan_stop()` expose the GAP radio procedures without hiding callbacks.

Use this as the BLE Core 0 lane. GATT services, characteristics, pairing policy, and storage callbacks stay in application code because those are product protocol choices.

The `ble` feature maps to `bt` and `nvs_flash`; the app still needs ESP-IDF Bluetooth/NimBLE Kconfig options enabled.

### `arc::net::Udp<Policy, Bus>`

Reusable Core 0 UDP transport plane.

UDP uses `arc::net::Radio` underneath and takes a radio lease while its Core 0 task is alive, so `Radio::off()` cannot destroy netif or Wi-Fi state beneath registered UDP event handlers.
`boot()` is idempotent while the transport is active. Call `Udp::stop(wait)` before `Radio::off()` when a program needs deep sleep or radio reconfiguration; it signals the task, wakes the Wi-Fi wait, closes the socket, unregisters Wi-Fi/IP handlers, releases the lease, then lets the caller reap the parked static task before that task memory is reused. A finite `wait` returns `ESP_ERR_TIMEOUT` if the task has not finished unwinding.

`Policy` supplies compile-time config:

- `tag`
- `stack`
- `ssid`
- `pass`
- `host`
- `port`
- Optional station constants: `auth`, `pmf`, `sae`, and `connect_retries`

Optional hooks:

- `start(bus)`
- `tick(bus, now)`

UDP holds one failed send as a pending frame before draining more events. This converts socket pressure into queue pressure instead of silently dropping a frame after `try_pop()`.
Set optional `Policy::stale` to a non-zero tick duration when stale pending telemetry should be discarded before reconnect sends it.

This lets you keep network code in the framework while still expressing program-specific control in the app.

Network is intentionally opt-in. Baseline apps only include `arc.hpp`; UDP apps add:

```cpp
#include "arc/udp.hpp"
```

### `arc::net::EspNow<Policy, Bus>`

Reusable Core 0 ESP-NOW plane.

ESP-NOW uses `arc::net::Radio` for the shared Wi-Fi base and takes a radio lease while its Core 0 task is alive. It owns ESP-NOW peer/callback setup while the shared radio owner protects Wi-Fi/netif teardown.
`boot()` is idempotent while the transport is active. Call `EspNow::stop(wait)` before `Radio::off()` when ESP-NOW must be torn down; it stops the task loop, unregisters ESP-NOW callbacks, deinitializes ESP-NOW, releases the radio lease, then lets the caller reap the parked static task before that task memory is reused.

`Policy` supplies compile-time config:

- `tag`
- `stack`
- `channel`
- `peer`

Optional hooks:

- `start(bus)`
- `tick(bus, now)`
- `recv(bus, peer, data, len)`

ESP-NOW holds one failed send as a pending frame before draining more events, matching the UDP backpressure contract.
Set optional `Policy::stale` to a non-zero tick duration when stale pending telemetry should be discarded before radio recovery sends it.

This is the transport to use when you want Wi-Fi silicon and low latency, but not IP, DHCP, or sockets.

### `arc::Wave<Pin, HalfUs, Mhz>`

Static CPU-owned square-wave generator when you want fixed compile-time timing and explicit spin control on Core 1.
The hot loop keeps its compile-time constants in registers behind an optimizer barrier instead of forcing volatile stack spills.

## Outputs

- App binary: `build/arc.bin`
- ELF: `build/arc.elf`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`
- Custom partition CSV: `partitions_16mb.csv`

Arc uses one build directory by default: `build/`.
If you keep a separate release profile in parallel, use `build-release/` deliberately.

## ESP-NOW Example

Arc ships a standalone raw-radio demo at `examples/espnow`.

```bash
cd examples/espnow
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/espnow
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Link = arc::Link<Edge, Control, 256>`
- `Core1 = arc::Plane<Pulse, ... , Link>`
- `Core0 = arc::net::EspNow<Radio, Link>`

This keeps the same Arc program shape as UDP, but replaces IP transport with raw ESP-NOW frames.

## Count Example

Arc ships a standalone pulse-counter demo at `examples/count`.

```bash
cd examples/count
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/count
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Source = arc::Burst<...>`
- `Meter = arc::Count<...>`

Add a jumper from the configured output pin to the configured input pin and the program will report the hardware pulse count without polling the pin in software.

## Trace Example

Arc ships a standalone RMT loopback capture demo at `examples/trace`.

```bash
cd examples/trace
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/trace
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Source = arc::Burst<...>`
- `Sink = arc::Trace<...>`

Add a jumper from the configured output pin to the configured input pin and the program will print the captured symbol widths from hardware RX.

## OTA Example

Arc also ships a standalone OTA-slot inspection demo at `examples/ota`.

```bash
cd examples/ota
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/ota
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::Ota::running()`, `boot()`, and `next()`
- rollback/pending-verify visibility without mutating flash
- `arc::Space::flash(...)` and `arc::Space::parts(...)` alongside OTA slot state

## Timer Example

Arc also ships a standalone hardware-timer demo at `examples/timer`.

```bash
cd examples/timer
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/timer
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Led = arc::Drive<... , arc::Core::core0>`
- `Tick = arc::Timer<1'000'000>`

The LED edge is driven by a GPTimer alarm ISR instead of a busy loop, while a tiny host task logs the free-running counter.

## CAN Example

Arc also ships a standalone TWAI/CAN self-test demo at `examples/can`.

```bash
cd examples/can
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/can
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Bus = arc::Can<tx, rx, bitrate, ..., self_test, loopback>`
- `Bus::frame(...)`
- `Bus::send_wait(...)`
- `Bus::recv(...)`

It uses self-test loopback so the example can run without an external CAN transceiver.

## Copy Example

Arc also ships a standalone DMA memcpy demo at `examples/copy`.

```bash
cd examples/copy
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/copy
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Dma = arc::Copy<8, 64, 0, arc::CopyBackend::ahb>`
- `src = arc::dmabuf<std::uint8_t>(4096)`
- `dst = arc::dmabuf<std::uint8_t>(4096)`

The CPU submits a 4096-byte transfer, computes the source checksum while DMA owns the destination, then finishes the exact transfer through `arc::Copy::finish_coherent(...)`. The normal cache handoff is correct by construction.

## DVP Example

Arc also ships a standalone LCD_CAM DVP input skeleton at `examples/dvp`.

```bash
cd examples/dvp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/dvp
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Cam = arc::Dvp<arc::DvpLines<...>, vsync, pclk, href, xclk>`
- `Cam::boot()`
- `Cam::buffer<std::uint16_t>(...)`

It does not call `grab()` automatically because the external camera sensor must be configured first.

## I80 Example

Arc also ships a standalone LCD_CAM Intel 8080 demo at `examples/i80`.

```bash
cd examples/i80
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/i80
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Bus = arc::I80Bus<arc::Lines<...>, dc, wr>`
- `Lcd = arc::I80<Bus, cs, 20'000'000>`
- `Lcd::buffer<std::uint16_t>(...)`
- `Lcd::color_coherent(ticket, 0x2C, frame)`
- `Lcd::finish(ticket)`

Use this when a display or parallel peripheral should stream through LCD_CAM/DMA instead of CPU-driven GPIO.

## Probe Example

Arc also ships a standalone cycle-counter demo at `examples/probe`.

```bash
cd examples/probe
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/probe
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Led = arc::Drive<4, 0, arc::Core::core1>`
- `arc::Probe::now()` around the hot path
- `arc::CycleStats` for min/avg/max
- `arc::SeqReg<Snapshot>` to report from Core 1 to Core 0

Use this to measure the real cycle cost of Arc hot-path calls on your board.

## Sigma Example

Arc also ships a standalone Sigma-Delta Modulator demo at `examples/sigma`.

```bash
cd examples/sigma
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/sigma
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Analog = arc::Sigma<4, 1'000'000, 0>`
- `Analog::start()` to own one SDM output channel
- `Analog::write(value)` to sweep pulse density between `-90` and `90`

Use this when the output should be generated by the SDM peripheral instead of a CPU loop. Add a low-pass filter if you want a smoother analog voltage.

## Sleep Example

Arc also ships a standalone deep-sleep demo at `examples/sleep`.

```bash
cd examples/sleep
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/sleep
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `ARC_RTC` retained state for a boot counter
- `arc::Sleep::cause()` and `arc::Sleep::woke(...)`
- `arc::Sleep::after<2'000'000>()`
- `arc::Sleep::deep()`

Use this when the firmware should spend most of its life asleep instead of burning cycles.

## Temp Example

Arc also ships a standalone internal temperature sensor demo at `examples/temp`.

```bash
cd examples/temp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/temp
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example composes:

- `Die = arc::Temp<-10, 80>`
- `Die::start()`
- `Die::milli()` for integer telemetry

Use this when firmware needs thermal awareness while the S3 is being driven hard.

## UDP Example

Arc ships a standalone network demo at `examples/udp`.

```bash
cd examples/udp
. ./env.sh
idf.py menuconfig
idf.py build flash monitor
```

For fish:

```fish
cd examples/udp
source ./env.fish
idf.py menuconfig
idf.py build flash monitor
```

The example is tuned to the same `N16R8` target and uses its own `examples/udp/partitions_16mb.csv`.

The example writes the program directly in `examples/udp/main/app_main.cpp`.

- `Link = arc::Link<Edge, Control, 256>`
- `Core1 = arc::Plane<Pulse, ... , Link>`
- `Core0 = arc::net::Udp<Udp, Link>`

This is the intended style for larger apps: the app composes framework utilities in `main.cpp`, while the heavy transport/runtime machinery stays inside `arc`.

## Space Example

Arc also ships a standalone capacity demo at `examples/space`.

```bash
cd examples/space
. ./env.sh
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/space
source ./env.fish
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows two things:

- baseline runtime capacity via `arc::Space::all(...)`, including the current image size and percent used inside the active OTA slot
- the effect of `arc::inram`, `arc::psram`, `arc::dmabuf`, and `arc::simdbuf` on free heap and capability-specific pools

## PWM Example

Arc also ships a standalone hardware-PWM demo at `examples/pwm`.

```bash
cd examples/pwm
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/pwm
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `app::boot()` staying tiny
- `arc::Pwm<...>::start()` owning the waveform
- no pinned task and no busy loop for a simple periodic LED output

## Pulse Example

Arc also ships a standalone MCPWM waveform-plus-capture demo at `examples/pulse`.

```bash
cd examples/pulse
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/pulse
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::Pulse<...>::start()` owning a `20 kHz` hardware waveform on `GPIO4`
- `arc::Capture<...>::start()` timestamping the same waveform on `GPIO5`
- period, high time, and low time reported without a CPU sampling loop

## Scope Example

Arc also ships a standalone ADC-DMA demo at `examples/scope`.

```bash
cd examples/scope
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/scope
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::Scope<...>::start()` owning a `40 kHz` ADC continuous stream on `GPIO4`
- `arc::Scope::pull(...)` returning parsed samples instead of raw driver bytes
- frame and overflow counters reported alongside `avg/min/max`

## SPI Example

Arc also ships a standalone SPI loopback demo at `examples/spi`.

```bash
cd examples/spi
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/spi
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::SpiBus<...>::boot()` bringing up `SPI2_HOST`
- `arc::Spi<...>::queue_coherent(...)` submitting one full-duplex loopback transfer
- `arc::Spi<...>::finish_coherent(...)` waiting on the exact queued transfer and making RX cache-safe
- DMA-capable TX/RX storage with no raw transaction lifetime plumbing in user code

## I2S Example

Arc also ships a standalone I2S duplex demo at `examples/i2s`.

```bash
cd examples/i2s
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/i2s
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::I2s<...>::boot()` allocating a duplex standard-mode lane
- `arc::I2s<...>::preload(span)`, `write(span)`, and `read(span)` moving data through DMA
- `arc::Result<std::size_t>` replacing mutable byte-count out parameters on the ergonomic path
- TX/RX event counters reported without user ISR plumbing

## DSP Example

Arc also ships a standalone compute-plane demo at `examples/dsp`.

```bash
cd examples/dsp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/dsp
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::simdbuf<float>(...)` making the buffer placement explicit
- `arc::dsp::*` kernels running on Core 1
- `arc::SeqReg` publishing compute snapshots back to Core 0

## Bridge Example

Arc also ships a standalone complementary-MCPWM demo at `examples/bridge`.

```bash
cd examples/bridge
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/bridge
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::Bridge<...>::start()` owning a complementary pair on `GPIO4` and `GPIO5`
- explicit rising and falling edge dead-time at the MCPWM hardware level
- a tiny `app::boot()` surface while the power-stage waveform stays entirely in silicon

## Store Example

Arc also ships a standalone typed-NVS demo at `examples/store`.

```bash
cd examples/store
. ./env.sh
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/store
source ./env.fish
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

The example shows:

- `arc::Store::boot()` as the one-time baseline init for NVS
- `arc::Store::load_or(...)` for a typed control word
- `arc::Store::save(...)` after mutation
- `arc::Store::save_string(...)` and `load_string(...)` for fixed-buffer text config
- `arc::Space::flash(...)` and `arc::Space::heap(...)` so persistence and capacity stay visible together

## CI

Arc now ships a build workflow at `.github/workflows/build.yml`.

It builds the root baseline and every directory under `examples/*`, then writes each app binary size into the GitHub Actions step summary so size regressions are visible on every push or PR.

If `idf.py build` fails in CI, the workflow now dumps the matching `build/log/idf_py_stdout_output_*` and `idf_py_stderr_output_*` files directly into the job log so the real compiler or linker error is visible instead of only the outer `ninja` failure.

The workflow also caches:

- the ESP-IDF checkout under `~/esp-idf`
- the installed Espressif toolchain and Python env under `~/.espressif`
- `~/.ccache` for repeated C/C++ builds

GitHub-hosted runners are still ephemeral, so host packages installed by `apt` do not persist across jobs. Arc now only installs host packages if they are actually missing on the runner.

Before any build runs, CI also executes `./tools/check-repo.sh`. That check fails if generated `sdkconfig` files are tracked, if docs regress to `idf.py set-target ...`, or if a project stops routing through the shared `esp32s3` lock in `cmake/arc-idf.cmake`.

CI also executes `./tools/host-tests.sh` before the ESP-IDF build. That host test binary compiles pure Arc logic against tiny ESP attribute stubs and exercises SPSC, MPSC under real producer contention, Fanin round-robin behavior, wire codecs, stream helpers, SeqReg snapshots, and DSP/FIR math without flashing hardware.

CI then executes `./tools/host-bench.sh`. The benchmark binary repeats correctness checks inside the timed paths and reports host-runner throughput for Arc SPSC/MPSC/SeqReg/DSP/codecs plus standard-library baselines where a fair local baseline exists. It is not marketed as an ESP32-S3 cycle benchmark or as an Arduino/raw-IDF shootout; it is a regression signal for algorithmic shape, accidental allocation, and gross host-side slowdowns before firmware builds start.

## Notes

- Global ESP-IDF is fully supported and is usually the cleaner setup.
- Local `esp-idf/` is still useful when you want a pinned checkout beside the firmware repo.
- Default config assumes `ESP32-S3 N16R8` with `16 MB` flash and `8 MB` Octal PSRAM.
- If your board LED is not on `GPIO48`, change `CONFIG_ARC_LED` or `CONFIG_ARC_UDP_LED` in `menuconfig`.
- `arc::Drive` is the default CPU-driven output path on ESP32-S3.
- `arc::Sense` gives the same dedicated path for deterministic input sampling.
- `arc::Pwm` is the right default for fixed periodic output when a hardware timer can own the waveform.
- `arc::Pulse` is the next lane up when you want a stronger waveform block than LEDC and still do not want the CPU owning edges.
- `arc::Bridge` is the right lane when the waveform controls a half-bridge, gate driver, or any complementary output that must respect dead-time.
- `arc::Capture` is the cleanest way to timestamp edges in hardware before reaching for a GPIO ISR.
- `arc::Scope` is the lane to reach for when analog sampling should move into DMA instead of a software read loop.
- `arc::I2cBus`, `arc::I2c`, and `arc::I2cSlave` are the lanes to reach for when register-style devices or peer controllers should stay on hardware I2C instead of GPIO bit-banging.
- `arc::SpiBus`, `arc::Spi`, and `arc::SpiSlave` are the lanes to reach for when a sensor, display, converter, or external controller should move bytes through the SPI peripheral instead of CPU-owned toggling.
- `arc::net::Eap` is the right opt-in lane for WPA2/WPA3 Enterprise; keep supplicant credentials out of plain PSK builds.
- `arc::I2s` is the right lane when the data is naturally framed and should live on a DMA-backed serial stream.
- `arc::Wave` is for deliberate CPU-owned edges, not for the common case of “just blink this periodically”.
- `arc::Reg` is better than a queue for latest-wins control words.
- For multi-field control words, prefer explicit fixed-width fields over C++ bitfields.
- `arc::Spsc` is better than a register when event history matters and ownership is one writer plus one reader.
- `arc::net::EspNow` is the transport to reach for before IP when you want radio latency and do not need routers.
- `arc::Ota` belongs on Core 0 with transports and storage, not in the realtime loop.
- `arc::Burst`, `arc::Trace`, and `arc::Count` are the first place to go when a pin-level job should move from CPU polling into dedicated hardware.
- `arc::Trax` is for internal CPU trace memory; do not use RMT capture terminology when the signal never leaves the core.
- `arc::Timer` is the right timebase when cycle counters are too core-local or when you need a true peripheral alarm.
- `arc::Mask` is for tiny deterministic sections, not for normal app structure.
- `arc::Tight` is the next step after `arc::App` when you need a masked step loop, not a normal forever-loop task body.
- `arc::SeqReg` is the right latest-snapshot lane once a control word no longer fits in `arc::Reg`.
- `arc::dmabuf`, `arc::cachebuf`, `arc::simdbuf`, `arc::ahbbuf`, and `arc::axibuf` are the right way to keep performance-critical heap placement explicit.
- `arc::dsp` is intentionally small: it is there to feed the compute plane, not to hide the math under a giant framework.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
