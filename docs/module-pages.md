# Module Page Index

Every public Arc header has a generated website page with purpose, fit, CMake feature, source landmarks, zero-start steps, and the closest build or runtime proof path.

Use this page when you know the header name. Use the [Module Guide](/modules) when you only know the problem area.

## Buses, Displays, And Data Capture

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/adc.hpp` | ADC pad descriptors and oneshot reads. | [Open](/modules/arc-adc) |
| `arc/any.hpp` | No-heap type erasure for slow-path GPIO/I2C/SPI/UART-style drivers. | [Open](/modules/arc-any) |
| `arc/can.hpp` | TWAI/CAN ownership, filters, TX/RX, and counters. | [Open](/modules/arc-can) |
| `arc/dvp.hpp` | LCD_CAM DVP camera capture. | [Open](/modules/arc-dvp) |
| `arc/i2c.hpp` | I2C master bus and device ownership. | [Open](/modules/arc-i2c) |
| `arc/i2c_slave.hpp` | I2C slave ownership. | [Open](/modules/arc-i2c_slave) |
| `arc/i2s.hpp` | Standard, TDM, and PDM I2S DMA lanes. | [Open](/modules/arc-i2s) |
| `arc/i80.hpp` | LCD_CAM Intel 8080 parallel output. | [Open](/modules/arc-i80) |
| `arc/otg.hpp` | Native USB OTG PHY ownership. | [Open](/modules/arc-otg) |
| `arc/pru.hpp` | PRU-style LCD_CAM/I2S descriptor output and parallel capture. | [Open](/modules/arc-pru) |
| `arc/rgb.hpp` | RGB LCD panel output and frame-buffer ownership. | [Open](/modules/arc-rgb) |
| `arc/scope.hpp` | ADC continuous DMA capture. | [Open](/modules/arc-scope) |
| `arc/sd.hpp` | SD/MMC FAT mount and raw sector access. | [Open](/modules/arc-sd) |
| `arc/sdio_slave.hpp` | SDIO slave coherent queue/finish semantics. | [Open](/modules/arc-sdio_slave) |
| `arc/spi.hpp` | SPI master bus/device, transfer tickets, and coherent queue/finish. | [Open](/modules/arc-spi) |
| `arc/spi_slave.hpp` | SPI slave queue/finish ownership. | [Open](/modules/arc-spi_slave) |
| `arc/uart.hpp` | UART ports, pins, framing, buffers, and runtime baud changes. | [Open](/modules/arc-uart) |
| `arc/usb.hpp` | USB Serial/JTAG byte IO. | [Open](/modules/arc-usb) |
| `arc/usb_device.hpp` | USB device descriptors, Chapter 9 state, class-facing FIFO bridges. | [Open](/modules/arc-usb_device) |
| `arc/usb_host.hpp` | USB host bring-up, polling, and transfer policy hooks. | [Open](/modules/arc-usb_host) |

## Crypto, Security, VM, And Sandbox

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/aes.hpp` | AES block, stream modes, and GCM. | [Open](/modules/arc-aes) |
| `arc/blackbox.hpp` | Sealed Merkle-linked flight-recorder payloads. | [Open](/modules/arc-blackbox) |
| `arc/cert_bundle.hpp` | Certificate-bundle policy hooks. | [Open](/modules/arc-cert_bundle) |
| `arc/chaos.hpp` | Bounded fault injection and postmortem logging. | [Open](/modules/arc-chaos) |
| `arc/cloak.hpp` | Timing/power side-channel policy hooks. | [Open](/modules/arc-cloak) |
| `arc/crypto_dma.hpp` | Hardware-to-hardware crypto descriptor job planning. | [Open](/modules/arc-crypto_dma) |
| `arc/hmac.hpp` | eFuse-keyed HMAC and temporary JTAG unlock. | [Open](/modules/arc-hmac) |
| `arc/hotpatch.hpp` | Executable payload loading and IRAM detour policy hooks. | [Open](/modules/arc-hotpatch) |
| `arc/hotswap.hpp` | Signed native, BPF, and WASM hot-swap staging policy. | [Open](/modules/arc-hotswap) |
| `arc/hypervisor.hpp` | Restricted partition planning and trap decisions. | [Open](/modules/arc-hypervisor) |
| `arc/interrupt_matrix.hpp` | Direct interrupt routing contracts. | [Open](/modules/arc-interrupt_matrix) |
| `arc/jit.hpp` | Bounded BPF-to-executable-word translation hooks. | [Open](/modules/arc-jit) |
| `arc/kyber.hpp` | Caller-buffer ML-KEM-shaped polynomial/KEM surfaces. | [Open](/modules/arc-kyber) |
| `arc/migrator.hpp` | WASM state snapshot, transport, and resume helpers. | [Open](/modules/arc-migrator) |
| `arc/mpi.hpp` | Move-only mbedTLS big integers. | [Open](/modules/arc-mpi) |
| `arc/nvs_crypto.hpp` | Encrypted-NVS policy hooks. | [Open](/modules/arc-nvs_crypto) |
| `arc/paillier.hpp` | Modular arithmetic surfaces for encrypted telemetry aggregation. | [Open](/modules/arc-paillier) |
| `arc/pms.hpp` | ESP32-S3 permission-control region facade. | [Open](/modules/arc-pms) |
| `arc/provisioning.hpp` | Provisioning-state wrappers. | [Open](/modules/arc-provisioning) |
| `arc/puf.hpp` | Entropy sampling, extraction, and key derivation hooks. | [Open](/modules/arc-puf) |
| `arc/secure_boot.hpp` | Secure-boot state and policy hooks. | [Open](/modules/arc-secure_boot) |
| `arc/sha.hpp` | Accelerated SHA hashing. | [Open](/modules/arc-sha) |
| `arc/sign.hpp` | Digital Signature peripheral operations. | [Open](/modules/arc-sign) |
| `arc/tee.hpp` | Secure/non-secure world assignment planning. | [Open](/modules/arc-tee) |
| `arc/vm.hpp` | BPF bytecode execution and sandbox helpers. | [Open](/modules/arc-vm) |
| `arc/wasm_aot.hpp` | Bounded WASM AOT translation policy surface. | [Open](/modules/arc-wasm_aot) |
| `arc/xts.hpp` | Encrypted flash read/write helpers. | [Open](/modules/arc-xts) |

## DSP, Control, ML, And Vision

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/acoustic_slam.hpp` | FMCW chirp, TDoA, range, and acoustic swarm helpers. | [Open](/modules/arc-acoustic_slam) |
| `arc/cnc.hpp` | Kinematics and no-allocation G-code parsing. | [Open](/modules/arc-cnc) |
| `arc/covert.hpp` | PWM/Sigma-Delta FSK symbol planning for experimental air-gap channels. | [Open](/modules/arc-covert) |
| `arc/digital_twin.hpp` | HIL plant stepping and encoder-output policy hooks. | [Open](/modules/arc-digital_twin) |
| `arc/dsp.hpp` | Dot/scale/mix/MAC, FIR, PID, biquad, SOS, and state-space kernels. | [Open](/modules/arc-dsp) |
| `arc/ecs.hpp` | Structure-of-arrays style entity/control data helpers. | [Open](/modules/arc-ecs) |
| `arc/foc.hpp` | BLDC field-oriented control and motor-control helpers. | [Open](/modules/arc-foc) |
| `arc/hil.hpp` | HIL-facing helper types and evidence surfaces. | [Open](/modules/arc-hil) |
| `arc/hls.hpp` | HLS-shaped fixed-loop kernel helpers. | [Open](/modules/arc-hls) |
| `arc/hyper_matrix.hpp` | Fixed probability tensor fusion for swarm observations. | [Open](/modules/arc-hyper_matrix) |
| `arc/isp.hpp` | Caller-buffer camera ISP kernels. | [Open](/modules/arc-isp) |
| `arc/kalman.hpp` | Deterministic Kalman correction helpers. | [Open](/modules/arc-kalman) |
| `arc/lifi.hpp` | Manchester optical symbol generation. | [Open](/modules/arc-lifi) |
| `arc/maglev.hpp` | Unstable state-space control surfaces. | [Open](/modules/arc-maglev) |
| `arc/matrix.hpp` | Fixed-size matrix math. | [Open](/modules/arc-matrix) |
| `arc/ml.hpp` | Fixed-shape tensor, dense, quantized dense, conv, pool, and inference surfaces. | [Open](/modules/arc-ml) |
| `arc/motion.hpp` | Bounded synchronized motion plans. | [Open](/modules/arc-motion) |
| `arc/nav.hpp` | ESKF and quaternion navigation helpers. | [Open](/modules/arc-nav) |
| `arc/sdr.hpp` | LCD_CAM pulse-stream preparation for SDR experiments. | [Open](/modules/arc-sdr) |
| `arc/simd.hpp` | Explicit S3-focused vector and image/math kernels. | [Open](/modules/arc-simd) |
| `arc/snn.hpp` | Spike-driven inference primitives. | [Open](/modules/arc-snn) |
| `arc/star_tracker.hpp` | Star centroid and catalog matching helpers. | [Open](/modules/arc-star_tracker) |
| `arc/vision.hpp` | Sobel, optical flow, visual servo, and related vision kernels. | [Open](/modules/arc-vision) |
| `arc/vslam.hpp` | Visual SLAM correction hooks. | [Open](/modules/arc-vslam) |
| `arc/wavefront.hpp` | Multichannel acoustic phase planning and synthesis. | [Open](/modules/arc-wavefront) |

## Detail Headers

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/detail/cold.hpp` | Internal cold-path annotations. | [Open](/modules/arc-detail-cold) |
| `arc/detail/owner.hpp` | Internal move-only ownership helpers. | [Open](/modules/arc-detail-owner) |
| `arc/detail/quant.hpp` | Internal quantized rounding helpers. | [Open](/modules/arc-detail-quant) |

## GPIO, Timing, And Power

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/bridge.hpp` | Complementary MCPWM pairs with dead-time and optional brake input. | [Open](/modules/arc-bridge) |
| `arc/burst.hpp` | RMT symbol output. | [Open](/modules/arc-burst) |
| `arc/capture.hpp` | MCPWM capture timestamps. | [Open](/modules/arc-capture) |
| `arc/clock.hpp` | Cycle counter and timing conversions. | [Open](/modules/arc-clock) |
| `arc/count.hpp` | PCNT pulse and quadrature counting. | [Open](/modules/arc-count) |
| `arc/drive.hpp` | Dedicated GPIO output. | [Open](/modules/arc-drive) |
| `arc/etm.hpp` | Event Task Matrix event/task/channel routing. | [Open](/modules/arc-etm) |
| `arc/flexroute.hpp` | GPIO matrix routing and policy-based spare-route repair. | [Open](/modules/arc-flexroute) |
| `arc/fuse.hpp` | eFuse, MAC, package, and secure-version reads. | [Open](/modules/arc-fuse) |
| `arc/gpio.hpp` | General GPIO ownership. | [Open](/modules/arc-gpio) |
| `arc/mask.hpp` | Tiny Xtensa interrupt masking guards. | [Open](/modules/arc-mask) |
| `arc/pm.hpp` | CPU/APB/no-light-sleep PM locks. | [Open](/modules/arc-pm) |
| `arc/power_governor.hpp` | Slack-based boost/release policy hooks. | [Open](/modules/arc-power_governor) |
| `arc/power_profiler.hpp` | Current and instruction-counter profiling hooks. | [Open](/modules/arc-power_profiler) |
| `arc/probe.hpp` | Cycle, jitter, deadline, and stall statistics. | [Open](/modules/arc-probe) |
| `arc/pulse.hpp` | MCPWM waveform generation. | [Open](/modules/arc-pulse) |
| `arc/pwm.hpp` | LEDC hardware PWM. | [Open](/modules/arc-pwm) |
| `arc/rng.hpp` | Hardware random bytes and values. | [Open](/modules/arc-rng) |
| `arc/rtc.hpp` | RTC GPIO and retained RTC helpers. | [Open](/modules/arc-rtc) |
| `arc/sense.hpp` | Dedicated GPIO input. | [Open](/modules/arc-sense) |
| `arc/sigma.hpp` | Sigma-Delta pulse-density output. | [Open](/modules/arc-sigma) |
| `arc/sleep.hpp` | Light/deep sleep entry and wake-source policy. | [Open](/modules/arc-sleep) |
| `arc/tdma.hpp` | Deterministic radio transmit windows. | [Open](/modules/arc-tdma) |
| `arc/temp.hpp` | Internal die-temperature sensor. | [Open](/modules/arc-temp) |
| `arc/time.hpp` | Global SYSTIMER-backed microsecond time. | [Open](/modules/arc-time) |
| `arc/timer.hpp` | GPTimer timebase and alarm hooks. | [Open](/modules/arc-timer) |
| `arc/timesync.hpp` | Peer clock discipline over fixed timestamp samples. | [Open](/modules/arc-timesync) |
| `arc/touch.hpp` | Capacitive touch controller and channel ownership. | [Open](/modules/arc-touch) |
| `arc/trace.hpp` | RMT symbol capture. | [Open](/modules/arc-trace) |
| `arc/wave.hpp` | CPU-owned square-wave generation with explicit timing-source policy. | [Open](/modules/arc-wave) |
| `arc/wdt.hpp` | Task watchdog setup, users, and feeds. | [Open](/modules/arc-wdt) |

## Lock-Free Lanes

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/fanin.hpp` | One SPSC lane per producer with round-robin draining. | [Open](/modules/arc-fanin) |
| `arc/log.hpp` | Binary event lane for realtime logs drained later by Core 0. | [Open](/modules/arc-log) |
| `arc/mpsc.hpp` | Many-producer/one-consumer fan-in with cache-line isolated or dense cells. | [Open](/modules/arc-mpsc) |
| `arc/postmortem.hpp` | RTC no-init reboot-surviving diagnostic ring. | [Open](/modules/arc-postmortem) |
| `arc/rcu.hpp` | Dual-buffer latest configuration handoff. | [Open](/modules/arc-rcu) |
| `arc/reg.hpp` | Single-word latest-wins values. | [Open](/modules/arc-reg) |
| `arc/rpc.hpp` | Typed request/reply lanes over fixed queues. | [Open](/modules/arc-rpc) |
| `arc/rtc_ring.hpp` | RTC-memory SPSC lane for ULP/main-core handoff. | [Open](/modules/arc-rtc_ring) |
| `arc/seq.hpp` | Seqlock-style latest snapshots for larger trivially copyable payloads. | [Open](/modules/arc-seq) |
| `arc/spsc.hpp` | One-producer/one-consumer queues, batch push/pop, and role endpoints. | [Open](/modules/arc-spsc) |

## Memory, DMA, And Placement

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/axi_graph.hpp` | Compile-time hardware graph planning over DMA endpoints and board trigger policy. | [Open](/modules/arc-axi_graph) |
| `arc/cache.hpp` | Explicit CPU-to-device and device-to-CPU cache ownership. | [Open](/modules/arc-cache) |
| `arc/cache_lock.hpp` | Policy facade for cache-locked hot code or data regions. | [Open](/modules/arc-cache_lock) |
| `arc/caps.hpp` | Capability-tagged buffers such as `dmabuf`, `simdbuf`, `rtbuf`, and allocators. | [Open](/modules/arc-caps) |
| `arc/copy.hpp` | Async DMA memcpy, exact tickets, and coherent copy leases. | [Open](/modules/arc-copy) |
| `arc/distributed_mmu.hpp` | Remote span fault planning and deterministic cache-line fetch policy. | [Open](/modules/arc-distributed_mmu) |
| `arc/dma_chain.hpp` | Static DMA descriptor rings and owned DMA-chain buffers. | [Open](/modules/arc-dma_chain) |
| `arc/fram.hpp` | External FRAM/MRAM offset allocation and typed persistence hooks. | [Open](/modules/arc-fram) |
| `arc/mmu_span.hpp` | Typed read-only spans over mapped flash or PSRAM data. | [Open](/modules/arc-mmu_span) |
| `arc/pipeline.hpp` | Descriptor endpoint composition and 2D row binding. | [Open](/modules/arc-pipeline) |
| `arc/place.hpp` | Section-placement aliases such as `ARC_HOT`, `ARC_DMA`, and `ARC_RTC`. | [Open](/modules/arc-place) |
| `arc/pmr.hpp` | Capability-aware polymorphic memory-resource hooks. | [Open](/modules/arc-pmr) |
| `arc/prefetch.hpp` | Explicit read/write prefetch hints for long memory walks. | [Open](/modules/arc-prefetch) |
| `arc/scrub.hpp` | CRC sealing and background scan state for fixed memory regions. | [Open](/modules/arc-scrub) |

## Network, Radio, And Wire Protocols

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/bft.hpp` | Bounded BFT vote collection and quorum certificates for fleet decisions. | [Open](/modules/arc-bft) |
| `arc/ble.hpp` | NimBLE lifecycle, GAP, advertising, and scanning bridge. | [Open](/modules/arc-ble) |
| `arc/ble_mesh.hpp` | BLE Mesh payload validation and policy hooks. | [Open](/modules/arc-ble_mesh) |
| `arc/coap.hpp` | CoAP datagram codec. | [Open](/modules/arc-coap) |
| `arc/crdt.hpp` | Heapless CRDT counters, registers, and fixed replicated-state frames. | [Open](/modules/arc-crdt) |
| `arc/csi.hpp` | Wi-Fi CSI capture and feature extraction. | [Open](/modules/arc-csi) |
| `arc/eap.hpp` | WPA2/WPA3 Enterprise setup for the shared STA radio. | [Open](/modules/arc-eap) |
| `arc/espnow.hpp` | Reusable ESP-NOW transport plane. | [Open](/modules/arc-espnow) |
| `arc/ethernet.hpp` | Raw Ethernet frame ring for policy-owned MAC/PHY paths. | [Open](/modules/arc-ethernet) |
| `arc/fabric.hpp` | Static TDMA mesh routing over ESP-NOW-style slots. | [Open](/modules/arc-fabric) |
| `arc/ftm.hpp` | Wi-Fi FTM ranging and multilateration helpers. | [Open](/modules/arc-ftm) |
| `arc/http.hpp` | ESP-IDF HTTP/HTTPS client RAII wrapper. | [Open](/modules/arc-http) |
| `arc/http_server.hpp` | Small no-heap HTTP request parser, router, and response builder. | [Open](/modules/arc-http_server) |
| `arc/ip.hpp` | IP readiness and address-family helpers. | [Open](/modules/arc-ip) |
| `arc/mdns.hpp` | mDNS host and service advertisement. | [Open](/modules/arc-mdns) |
| `arc/mqtt.hpp` | MQTT 3.1.1 codec and session helper. | [Open](/modules/arc-mqtt) |
| `arc/net.hpp` | Shared Wi-Fi radio base. | [Open](/modules/arc-net) |
| `arc/netrpc.hpp` | Struct-codec commands over radio or transport payloads. | [Open](/modules/arc-netrpc) |
| `arc/pack.hpp` | Fixed binary record schemas and struct codecs. | [Open](/modules/arc-pack) |
| `arc/pbuf.hpp` | RAII lwIP packet-buffer ownership. | [Open](/modules/arc-pbuf) |
| `arc/poll.hpp` | Heapless `select()` wrapper for caller-owned sockets. | [Open](/modules/arc-poll) |
| `arc/rdma.hpp` | Aligned raw Wi-Fi write-frame planning. | [Open](/modules/arc-rdma) |
| `arc/stream.hpp` | Exact byte streams, length-prefixed frames, and small stream erasure. | [Open](/modules/arc-stream) |
| `arc/swarm.hpp` | Distributed snapshots, schedules, and swarm helper types. | [Open](/modules/arc-swarm) |
| `arc/tcp.hpp` | Direct TCP client sockets. | [Open](/modules/arc-tcp) |
| `arc/thread.hpp` | Thread/OpenThread policy bridge. | [Open](/modules/arc-thread) |
| `arc/tls.hpp` | Direct ESP-TLS client sessions. | [Open](/modules/arc-tls) |
| `arc/tsn.hpp` | Time-aware Ethernet gate schedule checks for deterministic transmit windows. | [Open](/modules/arc-tsn) |
| `arc/udp.hpp` | Reusable Core 0 UDP transport plane. | [Open](/modules/arc-udp) |
| `arc/uri.hpp` | Heapless URI parsing and percent encode/decode. | [Open](/modules/arc-uri) |
| `arc/w5500.hpp` | Policy-driven W5500 raw Ethernet path. | [Open](/modules/arc-w5500) |
| `arc/ws.hpp` | WebSocket handshake and frame codec. | [Open](/modules/arc-ws) |

## Observability And Trace

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/mcap.hpp` | Fixed-buffer MCAP telemetry records. | [Open](/modules/arc-mcap) |
| `arc/perfetto.hpp` | Compact binary Perfetto trace records. | [Open](/modules/arc-perfetto) |
| `arc/trace_event.hpp` | Trace-event JSON fragments from binary log events. | [Open](/modules/arc-trace_event) |
| `arc/trace_live.hpp` | Half-full trace chunk handoff to policy-owned sinks. | [Open](/modules/arc-trace_live) |
| `arc/trace_stream.hpp` | Draining binary logs to UDP, WebSocket, file, or custom sinks. | [Open](/modules/arc-trace_stream) |
| `arc/trax.hpp` | Xtensa TRAX instruction trace memory control. | [Open](/modules/arc-trax) |

## Profile Modules

| Header | Purpose | Page |
| --- | --- | --- |
| `arc.hpp` | Compatibility umbrella that exposes SDK-backed feature headers only when their ESP-IDF headers are visible. | [Open](/modules/arc) |
| `arc/core.hpp` | Core task shape, topology, init, GPIO, timing, queues, text, and basic storage-neutral substrate pieces. | [Open](/modules/arc-core) |
| `arc/crypto.hpp` | AES, SHA, HMAC, signatures, MPI, XTS, Kyber, Paillier, PUF, secure boot, and related security helpers. | [Open](/modules/arc-crypto) |
| `arc/math.hpp` | DSP, SIMD, fixed matrices, Kalman, motion, ML, and control math surfaces. | [Open](/modules/arc-math) |
| `arc/memory.hpp` | Capability buffers, cache ownership, DMA copy, descriptor chains, AXI graphs, pipelines, scrubbing, and placement helpers. | [Open](/modules/arc-memory) |
| `arc/net_codecs.hpp` | URI, streams, fixed records, CRDTs, BFT votes, MQTT, WebSocket, CoAP, and small HTTP server helpers without owning Wi-Fi. | [Open](/modules/arc-net_codecs) |
| `arc/robotics.hpp` | Motor control, CNC, motion, sensors, vision, DVP/LCD, digital twin, and robotics-oriented hardware paths. | [Open](/modules/arc-robotics) |
| `arc/sandbox.hpp` | VM, JIT, WASM AOT, hypervisor, PMS/TEE planning, hotpatch, chaos, and sandbox policy hooks. | [Open](/modules/arc-sandbox) |

## Program Shape And Ownership

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/audit.hpp` | Opt-in misuse assertions for queues and topology-sensitive lanes. | [Open](/modules/arc-audit) |
| `arc/bare_core.hpp` | True-AMP Core 1 boot contracts for board policies outside FreeRTOS. | [Open](/modules/arc-bare_core) |
| `arc/bus.hpp` | Compatibility naming for shared event/control buses. | [Open](/modules/arc-bus) |
| `arc/claim.hpp` | Runtime hardware ownership claims. | [Open](/modules/arc-claim) |
| `arc/cli.hpp` | Fixed command parsing from caller-owned byte spans. | [Open](/modules/arc-cli) |
| `arc/coro.hpp` | Heapless coroutine state machines using explicit arenas. | [Open](/modules/arc-coro) |
| `arc/fence.hpp` | Small memory-ordering helpers used by lock-free paths. | [Open](/modules/arc-fence) |
| `arc/flow.hpp` | Static source-lane-sink data path composition. | [Open](/modules/arc-flow) |
| `arc/fsm.hpp` | Compile-time automata and transition-table checks. | [Open](/modules/arc-fsm) |
| `arc/init.hpp` | Boot-once and shared-reference init state machines. | [Open](/modules/arc-init) |
| `arc/ipc.hpp` | Emergency and cross-partition IPC policy surface. | [Open](/modules/arc-ipc) |
| `arc/lockstep.hpp` | Dual-output comparison and policy hooks for lockstep safety checks. | [Open](/modules/arc-lockstep) |
| `arc/plane.hpp` | Stateful pinned workloads with explicit shared state. | [Open](/modules/arc-plane) |
| `arc/roles.hpp` | Producer/consumer endpoint exposure without exposing root queue mutation. | [Open](/modules/arc-roles) |
| `arc/rtos.hpp` | Safe chrono-to-FreeRTOS tick conversion helpers. | [Open](/modules/arc-rtos) |
| `arc/sim.hpp` | Host simulator FIFO plus pin drive/sense facades for app logic tests. | [Open](/modules/arc-sim) |
| `arc/sketch.hpp` | Compatibility alias for small app-style programs. | [Open](/modules/arc-sketch) |
| `arc/stack.hpp` | Compile-time stack budget helpers and task stack floors. | [Open](/modules/arc-stack) |
| `arc/task.hpp` | Static FreeRTOS task memory and pinned task bring-up. | [Open](/modules/arc-task) |
| `arc/text.hpp` | Fixed-buffer text, JSON escaping, integer formatting, and `format_to`. | [Open](/modules/arc-text) |
| `arc/tight.hpp` | Masked deterministic step loops for the rare very-low-jitter path. | [Open](/modules/arc-tight) |
| `arc/topology.hpp` | One-file board pin topology checks through `arc::Pins`. | [Open](/modules/arc-topology) |
| `arc/watch.hpp` | Lightweight watch/check helpers for policy code. | [Open](/modules/arc-watch) |

## Storage And Update

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/file.hpp` | RAII `FILE*` ownership for mounted VFS paths. | [Open](/modules/arc-file) |
| `arc/flash_log.hpp` | Fixed-record queue flushed through a storage sink. | [Open](/modules/arc-flash_log) |
| `arc/flash_off.hpp` | Policy guard for flash/cache-off critical sections. | [Open](/modules/arc-flash_off) |
| `arc/fs.hpp` | SPIFFS and FAT-on-flash VFS mounts. | [Open](/modules/arc-fs) |
| `arc/ota.hpp` | Staged OTA write, slot state, confirm, and rollback. | [Open](/modules/arc-ota) |
| `arc/secure_update.hpp` | Decrypt/verify/write policy composition for encrypted OTA streams. | [Open](/modules/arc-secure_update) |
| `arc/space.hpp` | Flash, partition, image, and heap capacity reporting. | [Open](/modules/arc-space) |
| `arc/store.hpp` | Typed NVS blobs and fixed text config. | [Open](/modules/arc-store) |

## Target And Naming Contract

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/arch/riscv.hpp` | RISC-V architecture facts for experimental/ULP paths. | [Open](/modules/arc-arch-riscv) |
| `arc/arch/xtensa.hpp` | Xtensa-specific core and interrupt facts. | [Open](/modules/arc-arch-xtensa) |
| `arc/assume.hpp` | Optimizer and unreachable-code hints where a contract has already been checked. | [Open](/modules/arc-assume) |
| `arc/cfg.hpp` | Kconfig-backed Arc defaults used by examples and the root app. | [Open](/modules/arc-cfg) |
| `arc/concepts.hpp` | Small compile-time contracts for digital IO, buses, waves, and control results. | [Open](/modules/arc-concepts) |
| `arc/result.hpp` | `arc::Result<T>`, `arc::Status`, `ARC_TRY`, and `ARC_CHECK`. | [Open](/modules/arc-result) |
| `arc/sdk.hpp` | SDK-facing compatibility helpers. | [Open](/modules/arc-sdk) |
| `arc/soc.hpp` | Compile-time ESP32 target capability map. | [Open](/modules/arc-soc) |
| `arc/soc/esp32p4.hpp` | ESP32-P4 target facts. | [Open](/modules/arc-soc-esp32p4) |
| `arc/soc/esp32s3.hpp` | ESP32-S3 target facts. | [Open](/modules/arc-soc-esp32s3) |
| `arc/soc/esp32s31.hpp` | Experimental ESP32-S31 target facts. | [Open](/modules/arc-soc-esp32s31) |
| `arc/soc/target.hpp` | Short target-selection constants. | [Open](/modules/arc-soc-target) |

## ULP And Low-Power Coprocessor

| Header | Purpose | Page |
| --- | --- | --- |
| `arc/intermittent.hpp` | RTC no-init checkpoints for brownout/intermittent execution. | [Open](/modules/arc-intermittent) |
| `arc/ulp.hpp` | ULP RISC-V/FSM load, run, interrupt, and shared memory controls. | [Open](/modules/arc-ulp) |
| `arc/ulp_asm.hpp` | Compile-time ULP RISC-V program assembly. | [Open](/modules/arc-ulp_asm) |
| `arc/ulp_cxx.hpp` | Tiny C++ builder, GPIO/I2C/ADC, and SleepFsm-style ULP building blocks. | [Open](/modules/arc-ulp_cxx) |
| `arc/ulp_ml.hpp` | ULP-side int8 dense inference and semantic/audio wake helpers. | [Open](/modules/arc-ulp_ml) |
