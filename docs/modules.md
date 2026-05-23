# Module Guide

Arc is easier to read when you group the headers by job instead of reading the
file list alphabetically. Use this page as the map, then open the API reference
for exact method names and arguments.

Every public header also has its own generated module page with fit guidance,
the exact header name, CMake feature, source landmarks, zero-start steps, and
the closest proof path. Open the [Module Page Index](/module-pages) when you
want the per-header pages.

## How To Choose

Start with the problem, then pick the smallest module that owns that problem:

- use `arc/core.hpp` for the basic Core 0/Core 1 programming model;
- use `arc/memory.hpp` when buffers cross DMA, cache, PSRAM, or another core;
- use `arc/math.hpp` when Core 1 does fixed-shape math, DSP, or control work;
- use `arc/net_codecs.hpp` when bytes need MQTT, WebSocket, CoAP, XRCE, CRDT, BFT,
  URI, stream, or fixed binary record framing without a network task;
- use `arc/crypto.hpp`, `arc/robotics.hpp`, or `arc/sandbox.hpp` only when the
  app really needs those heavier domain groups;
- include a focused header such as `arc/spi.hpp` or `arc/pwm.hpp` when one
  application file only needs that lane.

In CMake, keep the same discipline. Call `arc_requires(...)` with the feature
names that match the headers you include. That keeps ESP-IDF components explicit
and avoids dragging every driver into every app.

## Profile Modules

These headers are the main entry points for readers and subset builds.

| Header | Use it for |
| --- | --- |
| `arc/core.hpp` | Core task shape, topology, init, GPIO, timing, queues, text, and basic storage-neutral substrate pieces. |
| `arc/memory.hpp` | Capability buffers, cache ownership, DMA copy, descriptor chains, AXI graphs, pipelines, scrubbing, and placement helpers. |
| `arc/net_codecs.hpp` | URI, streams, fixed records, CRDTs, BFT votes, MQTT, WebSocket, CoAP, XRCE, and small HTTP server helpers without owning Wi-Fi. |
| `arc/math.hpp` | DSP, SIMD, fixed matrices, Kalman, motion, ML, and control math surfaces. |
| `arc/crypto.hpp` | AES, SHA, HMAC, signatures, MPI, XTS, Kyber, Paillier, PUF, secure boot, and related security helpers. |
| `arc/robotics.hpp` | Motor control, CNC, motion, sensors, vision, DVP/LCD, digital twin, and robotics-oriented hardware paths. |
| `arc/sandbox.hpp` | VM, JIT, WASM AOT, hypervisor, PMS/TEE planning, hotpatch, chaos, and sandbox policy hooks. |
| `arc.hpp` | Compatibility umbrella that exposes SDK-backed feature headers only when their ESP-IDF headers are visible. |

## Target And Naming Contract

These headers describe what silicon and language surface Arc assumes.

| Header | Use it for |
| --- | --- |
| `arc/sdk.hpp` | SDK-facing compatibility helpers. |
| `arc/soc.hpp` | Compile-time ESP32 target capability map. |
| `arc/soc/target.hpp` | Short target-selection constants. |
| `arc/soc/esp32s3.hpp` | ESP32-S3 target facts. |
| `arc/soc/esp32p4.hpp` | ESP32-P4 target facts. |
| `arc/soc/esp32s31.hpp` | Experimental ESP32-S31 target facts. |
| `arc/arch/xtensa.hpp` | Xtensa-specific core and interrupt facts. |
| `arc/arch/riscv.hpp` | RISC-V architecture facts for experimental/ULP paths. |
| `arc/cfg.hpp` | Kconfig-backed Arc defaults used by examples and the root app. |
| `arc/result.hpp` | `arc::Result<T>`, `arc::Status`, `ARC_TRY`, and `ARC_CHECK`. |
| `arc/concepts.hpp` | Small compile-time contracts for digital IO, buses, waves, and control results. |
| `arc/assume.hpp` | Optimizer and unreachable-code hints where a contract has already been checked. |

## Program Shape And Ownership

Use these when deciding who owns work, lifetime, and access.

| Header | Use it for |
| --- | --- |
| `arc/task.hpp` | Static FreeRTOS task memory and pinned task bring-up. |
| `arc/borrow.hpp` | Static-lifetime read/mutable loans with core-owner access checks. |
| `arc/plane.hpp` | Stateful pinned workloads with explicit shared state. |
| `arc/sketch.hpp` | Compatibility alias for small app-style programs. |
| `arc/tight.hpp` | Masked deterministic step loops for the rare very-low-jitter path. |
| `arc/lockstep.hpp` | Dual-output comparison and policy hooks for lockstep safety checks. |
| `arc/sim.hpp` | Host simulator FIFO, SPI byte lane, trace log, harness ticks, and pin drive/sense facades for app logic tests. |
| `arc/bare_core.hpp` | True-AMP Core 1 boot contracts for board policies outside FreeRTOS. |
| `arc/coro.hpp` | Heapless coroutine state machines using explicit arenas. |
| `arc/rtos.hpp` | Safe chrono-to-FreeRTOS tick conversion helpers. |
| `arc/stack.hpp` | Compile-time stack budget helpers and task stack floors. |
| `arc/init.hpp` | Boot-once and shared-reference init state machines. |
| `arc/claim.hpp` | Runtime hardware ownership claims. |
| `arc/audit.hpp` | Opt-in misuse assertions for queues and topology-sensitive lanes. |
| `arc/roles.hpp` | Producer/consumer endpoint exposure without exposing root queue mutation. |
| `arc/topology.hpp` | One-file board pin topology checks through `arc::Pins`. |
| `arc/fsm.hpp` | Compile-time automata and transition-table checks. |
| `arc/flow.hpp` | Static source-lane-sink data path composition. |
| `arc/ipc.hpp` | Emergency and cross-partition IPC policy surface. |
| `arc/cli.hpp` | Fixed command parsing from caller-owned byte spans. |
| `arc/text.hpp` | Fixed-buffer text, JSON escaping, integer formatting, and `format_to`. |
| `arc/bus.hpp` | Compatibility naming for shared event/control buses. |
| `arc/fence.hpp` | Small memory-ordering helpers used by lock-free paths. |
| `arc/watch.hpp` | Lightweight watch/check helpers for policy code. |

## Memory, DMA, And Placement

Reach for these whenever a pointer crosses a hardware or core boundary.

| Header | Use it for |
| --- | --- |
| `arc/caps.hpp` | Capability-tagged buffers such as `dmabuf`, `simdbuf`, `rtbuf`, and allocators. |
| `arc/cache.hpp` | Explicit CPU-to-device and device-to-CPU cache ownership. |
| `arc/cache_lock.hpp` | Policy facade for cache-locked hot code or data regions. |
| `arc/copy.hpp` | Async DMA memcpy, exact tickets, and coherent copy leases. |
| `arc/dma_chain.hpp` | Static DMA descriptor rings and owned DMA-chain buffers. |
| `arc/axi_graph.hpp` | Compile-time hardware graph planning over DMA endpoints and board trigger policy. |
| `arc/pipeline.hpp` | Descriptor endpoint composition and 2D row binding. |
| `arc/mmu_span.hpp` | Typed read-only spans over mapped flash or PSRAM data. |
| `arc/distributed_mmu.hpp` | Remote span fault planning and deterministic cache-line fetch policy. |
| `arc/fram.hpp` | External FRAM/MRAM offset allocation and typed persistence hooks. |
| `arc/place.hpp` | Section-placement aliases such as `ARC_HOT`, `ARC_DMA`, and `ARC_RTC`. |
| `arc/prefetch.hpp` | Explicit read/write prefetch hints for long memory walks. |
| `arc/scrub.hpp` | CRC sealing and background scan state for fixed memory regions. |
| `arc/pmr.hpp` | Capability-aware polymorphic memory-resource hooks. |

## Lock-Free Lanes

These modules move data without FreeRTOS queues in the hot path.

| Header | Use it for |
| --- | --- |
| `arc/spsc.hpp` | One-producer/one-consumer queues, batch push/pop, and role endpoints. |
| `arc/mpsc.hpp` | Many-producer/one-consumer fan-in with cache-line isolated or dense cells. |
| `arc/fanin.hpp` | One SPSC lane per producer with round-robin draining. |
| `arc/reg.hpp` | Single-word latest-wins values. |
| `arc/seq.hpp` | Seqlock-style latest snapshots for larger trivially copyable payloads. |
| `arc/rcu.hpp` | Dual-buffer latest configuration handoff. |
| `arc/rpc.hpp` | Typed request/reply lanes over fixed queues. |
| `arc/rtc_ring.hpp` | RTC-memory SPSC lane for ULP/main-core handoff. |
| `arc/log.hpp` | Binary event lane for realtime logs drained later by Core 0. |
| `arc/postmortem.hpp` | RTC no-init reboot-surviving diagnostic ring. |

## GPIO, Timing, And Power

Use these to move pin edges, clocks, wake, and power policy into the right
hardware block.

| Header | Use it for |
| --- | --- |
| `arc/drive.hpp` | Dedicated GPIO output. |
| `arc/sense.hpp` | Dedicated GPIO input. |
| `arc/gpio.hpp` | General GPIO ownership. |
| `arc/flexroute.hpp` | GPIO matrix routing and policy-based spare-route repair. |
| `arc/pwm.hpp` | LEDC hardware PWM. |
| `arc/sigma.hpp` | Sigma-Delta pulse-density output. |
| `arc/pulse.hpp` | MCPWM waveform generation. |
| `arc/bridge.hpp` | Complementary MCPWM pairs with dead-time and optional brake input. |
| `arc/capture.hpp` | MCPWM capture timestamps. |
| `arc/burst.hpp` | RMT symbol output. |
| `arc/trace.hpp` | RMT symbol capture. |
| `arc/count.hpp` | PCNT pulse and quadrature counting. |
| `arc/timer.hpp` | GPTimer timebase and alarm hooks. |
| `arc/etm.hpp` | Event Task Matrix event/task/channel routing. |
| `arc/rtc.hpp` | RTC GPIO and retained RTC helpers. |
| `arc/sleep.hpp` | Light/deep sleep entry and wake-source policy. |
| `arc/pm.hpp` | CPU/APB/no-light-sleep PM locks. |
| `arc/time.hpp` | Global SYSTIMER-backed microsecond time. |
| `arc/clock.hpp` | Cycle counter and timing conversions. |
| `arc/timesync.hpp` | Peer clock discipline over fixed timestamp samples. |
| `arc/tdma.hpp` | Deterministic radio transmit windows. |
| `arc/wave.hpp` | CPU-owned square-wave generation with explicit timing-source policy. |
| `arc/probe.hpp` | Cycle, jitter, deadline, and stall statistics. |
| `arc/mask.hpp` | Tiny Xtensa interrupt masking guards. |
| `arc/power_governor.hpp` | Slack-based boost/release policy hooks. |
| `arc/power_profiler.hpp` | Current and instruction-counter profiling hooks. |
| `arc/temp.hpp` | Internal die-temperature sensor. |
| `arc/touch.hpp` | Capacitive touch controller and channel ownership. |
| `arc/rng.hpp` | Hardware random bytes and values. |
| `arc/wdt.hpp` | Task watchdog setup, users, and feeds. |
| `arc/fuse.hpp` | eFuse, MAC, package, and secure-version reads. |

## Buses, Displays, And Data Capture

These headers bind real ESP32-S3 peripherals to typed owners.

| Header | Use it for |
| --- | --- |
| `arc/any.hpp` | No-heap type erasure for slow-path GPIO/I2C/SPI/UART-style drivers. |
| `arc/i2c.hpp` | I2C master bus and device ownership. |
| `arc/i2c_slave.hpp` | I2C slave ownership. |
| `arc/spi.hpp` | SPI master bus/device, transfer tickets, and coherent queue/finish. |
| `arc/spi_slave.hpp` | SPI slave queue/finish ownership. |
| `arc/i2s.hpp` | Standard, TDM, and PDM I2S DMA lanes. |
| `arc/uart.hpp` | UART ports, pins, framing, buffers, and runtime baud changes. |
| `arc/usb.hpp` | USB Serial/JTAG byte IO. |
| `arc/otg.hpp` | Native USB OTG PHY ownership. |
| `arc/usb_device.hpp` | USB device descriptors, Chapter 9 state, class-facing FIFO bridges. |
| `arc/usb_host.hpp` | USB host bring-up, polling, and transfer policy hooks. |
| `arc/sdio_slave.hpp` | SDIO slave coherent queue/finish semantics. |
| `arc/sd.hpp` | SD/MMC FAT mount and raw sector access. |
| `arc/adc.hpp` | ADC pad descriptors and oneshot reads. |
| `arc/scope.hpp` | ADC continuous DMA capture. |
| `arc/can.hpp` | TWAI/CAN ownership, filters, TX/RX, and counters. |
| `arc/dvp.hpp` | LCD_CAM DVP camera capture. |
| `arc/i80.hpp` | LCD_CAM Intel 8080 parallel output. |
| `arc/rgb.hpp` | RGB LCD panel output and frame-buffer ownership. |
| `arc/pru.hpp` | PRU-style LCD_CAM/I2S descriptor output and parallel capture. |

## Storage And Update

Keep file, flash, and firmware update work on Core 0.

| Header | Use it for |
| --- | --- |
| `arc/fs.hpp` | SPIFFS and FAT-on-flash VFS mounts. |
| `arc/file.hpp` | RAII `FILE*` ownership for mounted VFS paths. |
| `arc/store.hpp` | Typed NVS blobs and fixed text config. |
| `arc/ota.hpp` | Staged OTA write, slot state, confirm, and rollback. |
| `arc/space.hpp` | Flash, partition, image, and heap capacity reporting. |
| `arc/flash_log.hpp` | Fixed-record queue flushed through a storage sink. |
| `arc/secure_update.hpp` | Decrypt/verify/write policy composition for encrypted OTA streams. |
| `arc/flash_off.hpp` | Policy guard for flash/cache-off critical sections. |

## Network, Radio, And Wire Protocols

The radio owner lives on Core 0. Protocol codecs stay caller-buffered.

| Header | Use it for |
| --- | --- |
| `arc/net.hpp` | Shared Wi-Fi radio base. |
| `arc/ip.hpp` | IP readiness and address-family helpers. |
| `arc/uri.hpp` | Heapless URI parsing and percent encode/decode. |
| `arc/tcp.hpp` | Direct TCP client sockets. |
| `arc/poll.hpp` | Heapless `select()` wrapper for caller-owned sockets. |
| `arc/pbuf.hpp` | RAII lwIP packet-buffer ownership. |
| `arc/tls.hpp` | Direct ESP-TLS client sessions. |
| `arc/http.hpp` | ESP-IDF HTTP/HTTPS client RAII wrapper. |
| `arc/http_server.hpp` | Small no-heap HTTP request parser, router, and response builder. |
| `arc/stream.hpp` | Exact byte streams, length-prefixed frames, and small stream erasure. |
| `arc/pack.hpp` | Fixed binary record schemas and struct codecs. |
| `arc/mqtt.hpp` | MQTT 3.1.1 codec and session helper. |
| `arc/ws.hpp` | WebSocket handshake and frame codec. |
| `arc/coap.hpp` | CoAP datagram codec. |
| `arc/xrce.hpp` | Fixed-buffer DDS-XRCE message and submessage framing. |
| `arc/crdt.hpp` | Heapless CRDT counters, registers, and fixed replicated-state frames. |
| `arc/bft.hpp` | Bounded BFT vote collection and quorum certificates for fleet decisions. |
| `arc/mdns.hpp` | mDNS host and service advertisement. |
| `arc/eap.hpp` | WPA2/WPA3 Enterprise setup for the shared STA radio. |
| `arc/udp.hpp` | Reusable Core 0 UDP transport plane. |
| `arc/espnow.hpp` | Reusable ESP-NOW transport plane. |
| `arc/csi.hpp` | Wi-Fi CSI capture and feature extraction. |
| `arc/ftm.hpp` | Wi-Fi FTM ranging and multilateration helpers. |
| `arc/fabric.hpp` | Static TDMA mesh routing over ESP-NOW-style slots. |
| `arc/rdma.hpp` | Aligned raw Wi-Fi write-frame planning. |
| `arc/netrpc.hpp` | Struct-codec commands over radio or transport payloads. |
| `arc/swarm.hpp` | Distributed snapshots, schedules, and swarm helper types. |
| `arc/ethernet.hpp` | Raw Ethernet frame ring for policy-owned MAC/PHY paths. |
| `arc/tsn.hpp` | Time-aware Ethernet gate schedule checks for deterministic transmit windows. |
| `arc/w5500.hpp` | Policy-driven W5500 raw Ethernet path. |
| `arc/thread.hpp` | Thread/OpenThread policy bridge. |
| `arc/ble.hpp` | NimBLE lifecycle, GAP, advertising, and scanning bridge. |
| `arc/ble_mesh.hpp` | BLE Mesh payload validation and policy hooks. |

## DSP, Control, ML, And Vision

These modules are for deterministic compute, not dynamic frameworks.

| Header | Use it for |
| --- | --- |
| `arc/dsp.hpp` | Dot/scale/mix/MAC, FIR, PID, biquad, SOS, and state-space kernels. |
| `arc/simd.hpp` | Explicit S3-focused vector and image/math kernels. |
| `arc/matrix.hpp` | Fixed-size matrix math. |
| `arc/kalman.hpp` | Deterministic Kalman correction helpers. |
| `arc/ml.hpp` | Fixed-shape tensor, dense, quantized dense, conv, pool, and inference surfaces. |
| `arc/snn.hpp` | Spike-driven inference primitives. |
| `arc/nav.hpp` | ESKF and quaternion navigation helpers. |
| `arc/foc.hpp` | BLDC field-oriented control and motor-control helpers. |
| `arc/motion.hpp` | Bounded synchronized motion plans. |
| `arc/cnc.hpp` | Kinematics and no-allocation G-code parsing. |
| `arc/maglev.hpp` | Unstable state-space control surfaces. |
| `arc/hls.hpp` | HLS-shaped fixed-loop kernel helpers. |
| `arc/wavefront.hpp` | Multichannel acoustic phase planning and synthesis. |
| `arc/digital_twin.hpp` | HIL plant stepping and encoder-output policy hooks. |
| `arc/hil.hpp` | HIL-facing helper types and evidence surfaces. |
| `arc/isp.hpp` | Caller-buffer camera ISP kernels. |
| `arc/vision.hpp` | Sobel, optical flow, visual servo, and related vision kernels. |
| `arc/vision_accel.hpp` | PPA/JPEG/H264 frame and bitstream plan validation. |
| `arc/vslam.hpp` | Visual SLAM correction hooks. |
| `arc/star_tracker.hpp` | Star centroid and catalog matching helpers. |
| `arc/ecs.hpp` | Structure-of-arrays style entity/control data helpers. |
| `arc/acoustic_slam.hpp` | FMCW chirp, TDoA, range, and acoustic swarm helpers. |
| `arc/hyper_matrix.hpp` | Fixed probability tensor fusion for swarm observations. |
| `arc/lifi.hpp` | Manchester optical symbol generation. |
| `arc/sdr.hpp` | LCD_CAM pulse-stream preparation for SDR experiments. |
| `arc/covert.hpp` | PWM/Sigma-Delta FSK symbol planning for experimental air-gap channels. |

## Crypto, Security, VM, And Sandbox

These are policy surfaces. Key custody, trust decisions, and product security
rules stay above Arc.

| Header | Use it for |
| --- | --- |
| `arc/aes.hpp` | AES block, stream modes, and GCM. |
| `arc/sha.hpp` | Accelerated SHA hashing. |
| `arc/hmac.hpp` | eFuse-keyed HMAC and temporary JTAG unlock. |
| `arc/sign.hpp` | Digital Signature peripheral operations. |
| `arc/mpi.hpp` | Move-only mbedTLS big integers. |
| `arc/xts.hpp` | Encrypted flash read/write helpers. |
| `arc/kyber.hpp` | Caller-buffer ML-KEM-shaped polynomial/KEM surfaces. |
| `arc/paillier.hpp` | Modular arithmetic surfaces for encrypted telemetry aggregation. |
| `arc/puf.hpp` | Entropy sampling, extraction, and key derivation hooks. |
| `arc/cloak.hpp` | Timing/power side-channel policy hooks. |
| `arc/blackbox.hpp` | Sealed Merkle-linked flight-recorder payloads. |
| `arc/cert_bundle.hpp` | Certificate-bundle policy hooks. |
| `arc/nvs_crypto.hpp` | Encrypted-NVS policy hooks. |
| `arc/secure_boot.hpp` | Secure-boot state and policy hooks. |
| `arc/pms.hpp` | ESP32-S3 permission-control region facade. |
| `arc/tee.hpp` | Secure/non-secure world assignment planning. |
| `arc/vm.hpp` | BPF bytecode execution and sandbox helpers. |
| `arc/jit.hpp` | Bounded BPF-to-executable-word translation hooks. |
| `arc/hotswap.hpp` | Signed native, BPF, and WASM hot-swap staging policy. |
| `arc/wasm_aot.hpp` | Bounded WASM AOT translation policy surface. |
| `arc/migrator.hpp` | WASM state snapshot, transport, and resume helpers. |
| `arc/hypervisor.hpp` | Restricted partition planning and trap decisions. |
| `arc/hotpatch.hpp` | Executable payload loading and IRAM detour policy hooks. |
| `arc/chaos.hpp` | Bounded fault injection and postmortem logging. |
| `arc/crypto_dma.hpp` | Hardware-to-hardware crypto descriptor job planning. |
| `arc/interrupt_matrix.hpp` | Direct interrupt routing contracts. |
| `arc/provisioning.hpp` | Provisioning-state wrappers. |

## ULP And Low-Power Coprocessor

Use these when the main cores should sleep while tiny policy code keeps running.

| Header | Use it for |
| --- | --- |
| `arc/ulp.hpp` | ULP RISC-V/FSM load, run, interrupt, and shared memory controls. |
| `arc/ulp_asm.hpp` | Compile-time ULP RISC-V program assembly. |
| `arc/ulp_cxx.hpp` | Tiny C++ builder, GPIO/I2C/ADC, and SleepFsm-style ULP building blocks. |
| `arc/ulp_ml.hpp` | ULP-side int8 dense inference and semantic/audio wake helpers. |
| `arc/lp_core.hpp` | ESP32-P4 LP-core entry tags, image metadata, and shared handoff lanes. |
| `arc/intermittent.hpp` | RTC no-init checkpoints for brownout/intermittent execution. |

## Observability And Trace

These modules make runtime evidence exportable without doing heavy work in the
hot loop.

| Header | Use it for |
| --- | --- |
| `arc/trace_event.hpp` | Trace-event JSON fragments from binary log events. |
| `arc/trace_stream.hpp` | Draining binary logs to UDP, WebSocket, file, or custom sinks. |
| `arc/trace_live.hpp` | Half-full trace chunk handoff to policy-owned sinks. |
| `arc/perfetto.hpp` | Compact binary Perfetto trace records. |
| `arc/mcap.hpp` | Fixed-buffer MCAP telemetry records. |
| `arc/trax.hpp` | Xtensa TRAX instruction trace memory control. |

## Detail Headers

Headers under `arc/detail/` are implementation support for public modules. Do
not start a new app from them.

| Header | Use it for |
| --- | --- |
| `arc/detail/cold.hpp` | Internal cold-path annotations. |
| `arc/detail/owner.hpp` | Internal move-only ownership helpers. |
| `arc/detail/quant.hpp` | Internal quantized rounding helpers. |

## Next Step

Once you know the module, open the API reference:

- [API Reference](/api)
- [Getting Started](/getting-started)
- [Architecture](/architecture)
- [Examples](/examples)
