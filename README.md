# Arc

[![build](https://github.com/dhimasardinata/arc/actions/workflows/build.yml/badge.svg)](https://github.com/dhimasardinata/arc/actions/workflows/build.yml)
[![docs](https://github.com/dhimasardinata/arc/actions/workflows/pages.yml/badge.svg)](https://github.com/dhimasardinata/arc/actions/workflows/pages.yml)
[![website](https://img.shields.io/badge/docs-dhimasardinata.github.io%2Farc-0f7c80.svg)](https://dhimasardinata.github.io/arc/)
[![license: AGPL-3.0-only](https://img.shields.io/badge/license-AGPL--3.0--only-blue.svg)](LICENSE)
[![commercial license: paid required](https://img.shields.io/badge/commercial%20license-paid%20required-a84f2b.svg)](COMMERCIAL-LICENSE.md)
![ESP-IDF v6.0.1](https://img.shields.io/badge/ESP--IDF-v6.0.1-E7352C.svg)
![C++ gnu++26](https://img.shields.io/badge/C%2B%2B-gnu%2B%2B26-00599C.svg)
![target ESP32-S3](https://img.shields.io/badge/target-ESP32--S3-222222.svg)

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

Website: `https://dhimasardinata.github.io/arc/`.

License: `AGPL-3.0-only`. Arc is open source, but intentionally strict copyleft: if you distribute modified firmware or run modified network-accessible services built from Arc, keep the corresponding Arc-covered source available under the same license terms. Proprietary or private-source Arc rights require a signed paid commercial agreement; cloning or embedding this repository does not create that grant.

Arc is not a convenience wrapper around ESP-IDF. It is a typed substrate for firmware that needs deterministic hot loops, explicit DMA/cache ownership, static task memory, and transport/protocol composition without hidden heap policy.

At a glance:

- Target: ESP32-S3 by default, explicit ESP32-P4, ESP-IDF `v6.0.1`, C++ `gnu++26`.
- Runtime shape: Core 0 owns control, networking, storage, logs, and framework work; Core 1 owns deterministic compute and GPIO.
- Allocation stance: hot paths use static storage, caller-owned spans, capability-tagged buffers, and no framework-owned protocol heap.
- Error stance: setup can return `esp_err_t` / `arc::Result<T>`; impossible topology and hot-path contract violations fail early.
- Transport stance: TCP, TLS, HTTP/HTTPS, UDP, ESP-NOW, MQTT, WebSocket, CoAP, and DDS-XRCE are composable lanes/codecs, not task-owning application frameworks.

## Why Arc

Arc exists for firmware where "it probably initializes" and "the ISR usually keeps up" are not acceptable engineering contracts.

| Problem | Arc's answer |
| --- | --- |
| Two files accidentally claim the same UART, SPI, I2C, or pin | Compile-time topology checks plus runtime claim tokens reject incompatible aliases before hardware corruption. |
| DMA buffers silently land in the wrong memory | Capability-tagged buffers and cache handoff APIs make SRAM/PSRAM/DMA ownership visible at the call site. |
| Realtime work competes with networking and logs | Core 1 is the deterministic compute plane; Core 0 owns control, transport, storage, and framework work. |
| Shared peripheral lifetime becomes informal | `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, and `arc::RefLease` encode init, rollback, references, and teardown in one atomic word. |
| Static shared state loses its lifetime story | `arc::StaticRef<&object, Core>`, `StaticLoan`, `StaticEdit`, and `LoanPack::with<Core>(fn)` keep global storage, core access, and conflicting static borrows visible in the type. |
| Lock-free code accidentally invokes C++ data-race UB | Queue and snapshot lanes use explicit acquire/release sequencing, cache-line layout, and dual-buffer handoff where payload tearing would otherwise be undefined. |
| ESP-IDF APIs leak raw handles into app code | Arc keeps ownership typed, static, and explicit while still returning `esp_err_t` or `arc::Result<T>` when recovery matters. |

The checked-in defaults are now tuned for `ESP32-S3 N16R8`:

- `16 MB` Quad SPI flash
- `8 MB` Octal PSRAM
- dual OTA app slots sized for a `16 MB` device

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- User programs live in `main/app_main.cpp`.
- `arc/core.hpp`, `arc/memory.hpp`, `arc/net_codecs.hpp`, `arc/math.hpp`, `arc/crypto.hpp`, `arc/robotics.hpp`, and `arc/sandbox.hpp` are profile entry points for subset builds; `arc.hpp` remains the compatibility umbrella and only exposes SDK-backed feature headers whose ESP-IDF components are actually in the build graph.
- `cmake/arc-deps.cmake` maps Arc feature names to ESP-IDF components so each app can stay explicit without writing a long `REQUIRES` list by hand.
- `arc::Soc` exposes the ESP32-S3 capability map from `soc_caps.h` as compile-time constants and hard-fails on non-S3 targets.
- `arc::Drive` and `arc::Sense` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::Pins` gives one-file board topology checks so duplicate physical pins are caught where hardware truth is declared.
- `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, `arc::Gate`, and `arc::MutexGate` provide initialization, shared-resource lifetime, and task-level locking primitives for static drivers.
- `arc::StaticRef<&object, Core>` creates zero-cost `StaticLoan` read/mutable views, while `arc::StaticEdit<WriteRef, ReadRefs...>` and `arc::LoanPack<...>` reject static borrow packs that mix a mutable loan with any other loan to the same object.
- `arc::stack` turns task stack sizing into a compile-time contract: Arc task wrappers reject stacks below their declared budget, and Arc's CMake interface enables GCC stack/frame errors for oversized C++ frames.
- `arc::Result<T>` is an opt-in `std::expected<T, esp_err_t>` alias for runtime operations that should not hard-panic.
- `arc::Cache` makes DMA/PSRAM cache coherency explicit at the call site, while `arc::Copy::send_coherent(...)` and `finish_coherent(...)` cover the common async-memcpy handoff without blocking useful CPU work.
- `arc::prefetch(...)` and `arc::prefetch_write(...)` expose explicit cache preloading hints for long PSRAM/DMA walks.
- `arc::Can` binds the ESP32-S3 TWAI/CAN controller with ISR-backed RX handoff.
- `arc::Burst` streams prebuilt RMT symbols with optional hardware looping.
- `arc::Trace` captures RMT symbols back into SRAM without a CPU sampling loop.
- `arc::Trax` controls Xtensa TRAX instruction trace memory for on-core execution history when TRAX is enabled in Kconfig.
- `arc::Pulse` uses MCPWM for higher-grade waveform generation than LEDC when period and edge placement matter.
- `arc::Bridge` drives complementary MCPWM pairs with explicit dead-time and optional hardware brake input.
- `arc::Capture` timestamps edges in hardware through the MCPWM capture block.
- `arc::matrix::FlexRoute` wraps GPIO matrix in/out policy hooks and reroutes failed PWM signals to spare pads without rebooting the app.
- `arc::Etm` owns ESP32-S3 Event Task Matrix channels so peripheral events can trigger peripheral tasks without waking a CPU, while `arc::EtmRoute` keeps the event, task, and channel lifetime bound together.
- `arc::AdcBus`, `arc::AdcOne`, and `arc::Scope` cover calibrated ADC oneshot reads and continuous DMA capture without mixing the ownership models.
- `arc::Copy` offloads memory movement to the async DMA memcpy engine.
- `arc::Dvp` captures parallel camera frames through the ESP32-S3 LCD_CAM DMA path.
- `arc::I80Bus` and `arc::I80` drive the ESP32-S3 LCD_CAM Intel 8080 DMA path with exact transfer tickets.
- `arc::Rgb` binds the ESP32-S3 RGB LCD engine with explicit timing, frame-buffer ownership, and refresh control.
- `arc::I2cBus`, `arc::I2c`, and `arc::I2cSlave` bind ESP32-S3 I2C master/slave buses and devices with recoverable init paths.
- `arc::SpiBus`, `arc::Spi`, and `arc::SpiSlave` drive DMA-capable SPI master/slave transfers with ticketed queue/finish ownership.
- `arc::AnyOut`, `arc::AnyIn`, `arc::AnyI2c`, `arc::AnySpi`, and `arc::AnyUart` provide no-heap type erasure for slow-path sensor/config/modem drivers that should compile out-of-line instead of becoming fully templated headers.
- `arc::I2s` owns standard-mode I2S channels, `arc::I2sTdm` covers multichannel framed lanes, and `arc::I2sPdm` covers one-line PDM RX/TX, with both raw `esp_err_t` and opt-in `arc::Result` APIs.
- `arc::Uart` binds ESP32-S3 UART ports, pins, framing, and buffers with fixed storage and typed read/write APIs.
- `arc::Usb` binds the ESP32-S3 USB Serial/JTAG lane with typed byte IO.
- `arc::Otg` owns the native USB OTG PHY for host/device stack bring-up without pretending to be a USB class framework.
- `arc::Sd` mounts ESP32-S3 SD/MMC cards through FAT and keeps raw sector access explicit.
- `arc::Fs` mounts SPIFFS, FAT-on-flash, and optional LittleFS VFS paths with fixed handle ownership.
- `arc::File` gives RAII VFS/POSIX file I/O without leaking `FILE*` ownership into app code.
- `arc::Count`, `arc::Quadrature`, and `arc::Encoder` offload pulse and quadrature accumulation to the PCNT block.
- `arc::dsp` adds hot-loop math kernels, FIR, PID, biquad, FFT, beamforming, and acoustic echo-cancellation primitives that pair naturally with `arc::simdbuf` and Core 1.
- `arc::Foc` combines Clarke/Park math, PID current loops, RCU targets, and centered PWM duty generation for static BLDC control loops.
- `arc::DualFoc`, `BridgeCarrierSync`, `FocEtmCurrentTrigger`, and `FocEncoderFusion` extend motor control toward dual-axis, carrier-synchronized, ETM-triggered, encoder-fused loops.
- `arc::MotionPlan` emits bounded Bresenham step masks into caller-owned buffers for synchronized multi-axis motion.
- `arc::cnc::Kinematics` and `arc::cnc::GCode` turn CoreXY, delta, and five-axis toolpaths into caller-owned `MotionPlan` buffers from zero-allocation G-code frames.
- `arc::dsp::Matrix`, `arc::dsp::Kalman`, and `arc::dsp::Lqr::adapt(...)` add fixed-size matrix math, sensor-fusion correction, and single-step adaptive LQR model updates without allocation.
- `arc::dsp::DspAccel`, `duty_svpwm`, `PllObserver`, `SlidingModeObserver`, and `arc::SCurve` extend the motor-control plane for accelerated kernels and smoother trajectories.
- `arc::simd::float32x4_t`, `int8x16_t`, `uint8x16_t`, `dot_s8`, `arc::simd::Rgb565::from_yuv422`, and `fft_radix2` expose explicit S3-focused math kernels that should not rely on auto-vectorization alone.
- `arc::ml::Tensor`, `Dense`, `QuantDenseS8`, `Conv2dS8`, `DepthwiseConv2dS8`, `MaxPool2d`, `mapped_weights`, and `Core1Inference` provide a zero-allocation inference surface for fixed-shape models stored in flash/PSRAM spans.
- `arc::BareCore<Program>` defines the true-AMP Core 1 boot contract for board policies that hold APP CPU outside FreeRTOS and jump directly into a static control loop.
- `arc::power::Intermittent` stores dying-gasp CPU, Tight-loop stack, and RCU state bytes in RTC no-init storage so a board policy can resurrect after brownout.
- `arc::FramArena<N>`, `FramRef<T>`, and `FramAlloc<N>` carve persistent FRAM/MRAM offsets and delegate typed load/store to board policy.
- `arc::DmaChain<N>` builds static scatter-gather descriptor rings for CPU-free waveform, display, and camera pipelines; `try_bind(...)` rejects invalid descriptor indexes, empty spans, descriptor lengths that cannot fit the hardware field, and cache-unsafe `arc::CacheLines` bindings, while `arc::OwnedDmaChain<T, N>` keeps DMA-capable buffers owned beside the descriptors.
- `arc::AxiGraph<Nodes...>` plans camera/crypto/SPI-style hardware graphs over DMA endpoints, links descriptor boundaries, and delegates ETM/AXI trigger setup to board policy.
- `arc::Pipeline`, `DmaEndpoint`, `Dma2dWindow`, and `bind_rows(...)` compose descriptor rings and 2D frame windows without making DMA topology implicit.
- `arc::PruOut`, `PruIn`, `PruTiming`, and `PruCursor` describe PRU-style LCD_CAM/I2S DMA waveform output and parallel capture rings for protocols the ESP32-S3 does not expose as a named peripheral.
- `arc::sdr::PulseSynth` and `Tx` turn caller-owned audio or bit spans into 1-bit LCD_CAM pulse streams for AM/FM/OOK software-defined radio experiments.
- `arc::dsp::Wavefront` computes 3D acoustic focus phases and synthesizes 16+ channel ultrasonic frames for `arc::I2sTdm` output.
- `arc::CryptoDma` describes hardware-to-hardware AES/GCM/SHA descriptor jobs so board policies can wire GDMA lanes without payload copies, with optional scoped leases for completion ownership.
- `arc::MmuSpan<T>` maps flash/PSRAM-backed regions into typed read-only spans for lookup tables, samples, and model weights.
- `arc::Task<void>` provides heapless coroutine state machines when the coroutine frame is allocated from an explicit `arc::TaskArena`.
- `arc::fsm::Automaton` synthesizes typed transition tables and rejects unreachable or non-terminal dead-end states at compile time.
- `arc::Cli` parses fixed command sets from `std::span<const char>` with `std::from_chars` and no mutable line buffer.
- `arc::Text` and `arc::format_to(...)` build decimal, hex, JSON-escaped, and common placeholder-formatted text into caller-owned buffers without formatting heap use.
- `arc::CacheLock` is a policy facade for locking hot code/data regions into cache when a board needs cache-miss-free control loops.
- `arc::HotPatch`, `HotPatchImage`, and `HotPatchDetour` load caller-provided PIE payload bytes into executable memory and install policy-encoded IRAM detours under `arc::Silence`.
- `arc::RtcRing<T, Capacity>` gives the ULP RISC-V and Xtensa cores a trivially copyable SPSC lane in RTC-capable storage.
- `arc::WorldGuard`, `HardwareGuard`, and `ICacheLock` expose policy-based hooks for S3 isolation, watchpoints, and instruction-cache pinning without baking private register layouts into public headers.
- `arc::TeePlan` and `TrustedObject` formalize secure/non-secure world assignment for memory, cores, and critical peripherals.
- `arc::vm::BPF`, `BpfInsn`, and `BpfSandbox` run bounded zero-allocation bytecode against caller-owned memory and expose a WorldGuard-backed protection hook for downloaded control blocks.
- `arc::vm::Jit` translates supported BPF ALU/control instructions into caller-owned executable words through a policy micro-assembler hook.
- `arc::vm::HotSwap` stages signed native, BPF, or WASM control payloads through explicit verify, protect, activate, and monotonic version-gate hooks.
- `arc::vm::Hypervisor` maps untrusted Core 0 partitions into restricted worlds, binds trap policy hooks, and returns explicit emulate/resume/terminate decisions.
- `arc::crypto::Cloak` adds policy hooks for randomized stalls, dummy reads, and inverse-load balancing around critical crypto/ML sections.
- `arc::covert::BlackBox` seals flight-recorder payloads into Merkle-linked nodes and delegates encrypted flash writes to board policy.
- `arc::x509::Bundle`, `arc::NvsCrypto`, and `arc::secure::SecureBoot` expose certificate-bundle, encrypted-NVS, and secure-boot policy hooks without owning product key lifecycle.
- `arc::crypto::Kyber` adds zero-allocation ML-KEM-shaped keypair, encapsulation, decapsulation, polynomial, NTT, and pointwise surfaces over caller-owned spans or `arc::CapsBuf`.
- `arc::crypto::Puf` samples SRAM/ADC startup entropy, extracts stable bits, and derives SHA-256 keys when the SHA accelerator headers are available.
- `arc::crypto::Paillier` composes `exp_mod` and `mul_mod` style big-integer backends for privacy-preserving encrypted telemetry aggregation.
- `arc::InterruptMatrix` and `RawVector` expose direct interrupt routing contracts for board-specific low-latency ISR stubs.
- `arc::pack::Overlay<Codec>` reads endian-correct fields directly from DMA/network spans without copying into a local struct.
- `arc::ulp::riscv::assemble(...)` emits tiny ULP RISC-V programs as compile-time `std::array<uint32_t, N>` blobs.
- `arc::ulp::Builder`, `Gpio`, `Adc`, `I2c`, and `SleepFsm` provide policy-based C++ building blocks for tiny ULP-side sensing and wake decisions.
- `arc::ulp::ml::QuantDenseS8` and `SemanticWake` run stripped-down int8 inference over ULP I2C samples before deciding whether the main cores should wake.
- `arc::ulp::ml::AudioSignature` and `AudioSignatureWake` turn small ADC/PDM sample windows into ULP-side signature bins before wake decisions.
- `ARC_LP_CORE`, `arc::lp::Image`, `Control`, `Word`, and `Mailbox<T>` mark ESP32-P4 LP-core entry/data surfaces and keep host/LP handoff seqlock-backed.
- `arc::isp::Debayer` and `AecAwb` turn raw camera spans into RGB565 frames and exposure/white-balance feedback without owning camera policy.
- `arc::vision::Sobel`, `OpticalFlow`, `VSlam`, and `VisualServo` add caller-buffer vision kernels, SIMD FAST corners, essential-matrix deltas, and ESKF correction hooks for the control plane.
- `arc::vision::PpaSrm`, `PpaFill`, `PpaBlend`, `JpegEncoder`, and `H264Encoder` validate zero-copy P4 vision-accelerator plans before board policy submits them.
- `arc::vision::StarTracker` thresholds DMA camera frames, extracts sub-pixel centroids, and matches triangle signatures against `arc::MmuSpan` star catalogs.
- `arc::LogLane` gives Core 1 a lock-free binary event lane that Core 0 can drain and format later.
- `arc::TraceEventWriter` turns binary log events into Chrome/Perfetto trace-event JSON fragments from caller-owned buffers.
- `arc::TraceStream` drains binary log lanes into a caller-provided UDP/WebSocket/file sink as live Perfetto JSON chunks.
- `arc::PerfettoWriter` emits compact binary trace records when JSON bandwidth is too expensive.
- `arc::mcap::Writer` emits fixed-buffer MCAP schema, channel, message, and footer records for ROS2-style telemetry handoff.
- `arc::xrce::Writer` frames fixed DDS-XRCE message and submessage bytes for UDP/serial micro-ROS bridges without owning a session task.
- `arc::net::Rdma` plans aligned raw Wi-Fi write frames so promiscuous RX policy can apply zero-copy remote state updates.
- `arc::mmu::DistributedPager` turns LoadStore-style remote span faults into fixed cache-line fetch/remap/resume policy calls.
- `arc::swarm::HyperMatrix` fuses VSlam, RF FTM, and AcousticSlam observations into a fixed 6D probability tensor and publishes the shared state through RDMA frames.
- `arc::trace::LiveStream` arms a half-full trace watermark, swaps banks, and hands trace chunks to USB/Ethernet sink policy without heap ownership.
- `arc::power::Governor` feeds `DeadlineStats` slack into a fixed Kalman predictor before applying CPU boost/release policy hooks.
- `arc::power::Profiler` interleaves 40 kHz current-shunt/INA219 samples with instruction PCs and emits compact Perfetto counter JSON for microjoule heatmaps.
- `arc::vm::WasmAot` translates bounded WASM op spans into caller-owned executable words and pairs with `WasmSandbox` PMS protection.
- `arc::swarm::AnyIdleCore` selects an online non-overrun peer from fixed fleet state, and `arc::swarm::Migrator` snapshots WASM linear memory, ships it through board radio policy, and resumes it inside that peer sandbox.
- `arc::net::Thread`, `arc::ble::Mesh`, `arc::SdioSlave`, and `arc::Ipc` wrap missing ESP-IDF edges behind explicit policy hooks.
- `arc::optical::LiFi` builds Manchester optical symbols for RMT/ETM-style LED and photodiode air-gap links.
- `arc::hil::DigitalTwin` closes a silicon-in-the-loop plant model by sampling capture ticks, stepping `StateSpace`, forecasting fixed horizons from copied state, and emitting encoder policy output.
- `arc::PostmortemFaultFrame` stores compact hard-fault registers and stack PCs in RTC memory for reboot-time reporting.
- `arc::Probe`, `arc::CycleStats`, `arc::JitterStats`, `arc::DeadlineStats`, and `arc::StallStats` read the Xtensa cycle counter so hot-path latency, period jitter, control-loop budget slack, and shared-bus stalls can be measured, not guessed.
- `arc::proof::Pack` carries compile-time proof facts, subject-specific queries, and verified cycle budgets beside workloads, giving release tooling a small proof-carrying metadata surface without heap or RTTI.
- `arc::Mask` gives an explicit Core-local interrupt barrier when you really need to silence OS-visible interrupts around a tiny hot section.
- `arc::Pwm` binds LEDC hardware PWM directly to compile-time pin/frequency defaults with runtime duty retuning.
- `arc::Sigma` binds the ESP32-S3 Sigma-Delta Modulator to a compile-time GPIO and sample rate with runtime density retuning.
- `arc::covert::EmTx` and `SonicTx` turn PWM or Sigma-Delta lanes into caller-planned FSK emitters for air-gapped EM or ultrasonic signaling.
- `arc::swarm::AcousticSlam` emits FMCW chirps, extracts TDoA from microphone spans, solves 3D acoustic ranges, and feeds Kalman position filters.
- `arc::chaos::Monkey` injects bounded bit flips, I2C stalls, ESP-NOW drops, and Tight-loop overruns while logging events to `arc::Postmortem`.
- `arc::Timer` binds the GPTimer block to a compile-time timebase and optional ISR hook.
- `arc::Sleep` wraps wake causes, timer wake, power-domain policy, light sleep, and deep sleep.
- `arc::Pm` gives typed ESP-IDF PM locks for CPU/APB/no-light-sleep critical sections when DFS is enabled.
- `arc::Rng` exposes the ESP32-S3 hardware random source for fixed buffers and typed values.
- `arc::Time` reads the global SYSTIMER-backed microsecond clock for cross-core timestamps.
- `arc::PtpClock` disciplines nanosecond hardware timestamp samples for sub-microsecond TSN-style node synchronization.
- `arc::net::TsnSchedule<N>` evaluates fixed time-aware Ethernet gate windows from a synchronized nanosecond clock.
- `arc::rtos` converts `std::chrono` durations to bounded FreeRTOS millisecond/tick values for APIs that sleep or wait.
- `arc::Sha` hashes buffers through Espressif's accelerated PSA/mbedTLS SHA path with `arc::Result` output.
- `arc::Aes` and `arc::Gcm` wrap AES block/stream/GCM operations with explicit key setup and caller-owned buffers.
- `arc::Hmac` computes eFuse-keyed SHA-256 HMACs and gates temporary JTAG unlock through the HMAC peripheral.
- `arc::Sign` drives the Digital Signature peripheral for eFuse-key-backed RSA signatures.
- `arc::Mpi` owns mbedTLS big integers and uses ESP-IDF's MPI/RSA accelerator path when the target Kconfig enables it.
- `arc::Xts` exposes hardware encrypted flash reads/writes and encrypted-partition alignment checks.
- `arc::Pms` is a policy facade for ESP32-S3 permission-control region locking without hard-coding unstable private registers into public APIs.
- `arc::SecureUpdate` composes decrypt/verify/write policies for encrypted OTA streams without owning transport buffers.
- `arc::FlashOff` is a policy guard for flash/cache-off critical regions; the app supplies the chip-specific enter/leave implementation.
- `arc::Wdt` exposes explicit task-watchdog configuration, task/user subscription, and feeding.
- `arc::Fuse` reads eFuse fields, blocks, MACs, package, and secure-version state.
- `arc::Ulp` loads and runs ULP RISC-V or FSM binaries and gives RTC-shared atomic words plus seqlock-style shared payloads for main-core handoff.
- `arc::Temp` reads the ESP32-S3 internal temperature sensor for thermal telemetry.
- `arc::TouchBus` and `arc::Touch` bind the ESP32-S3 capacitive touch controller and channels with explicit scan, filter, wake, and channel-data ownership.
- `arc::Tight` runs a masked per-step loop with optional cycle-budget overrun telemetry for the rare path that needs tighter jitter than `arc::App`.
- `arc::Lockstep<T>` compares redundant Core 0/Core 1 outputs and calls policy capture/halt hooks before rejecting mismatched ticks.
- `arc::Scrub<N>` seals fixed memory regions and lets a background task catch silent IRAM/DRAM/PSRAM corruption with policy hooks.
- `arc::sim::Drive`, `Sense`, `Spi`, `Fifo`, `TraceLog`, and `Harness` give host builds a fixed simulator surface for pin output traces, virtual input samples, SPI byte exchanges, and deterministic SILS timelines.
- `arc::App` runs a tiny zero-cost program on a chosen core.
- `arc::Link` gives shared event/control state without heap or virtual dispatch.
- `arc::Flow<Source, Lane, Sink>` wires one static source/lane/sink blueprint with compile-time payload compatibility, while `arc::Spsc` gives a bounded lock-free lane for one producer and one consumer; `arc::Roles<Lane>` owns a lane privately when you want producer/consumer endpoints to be the only compile-time API, and `PushRole`/`PopRole` let templates require just one endpoint side; `arc::Ring` remains the terse compatibility alias.
- `arc::Audit<Topology>` adds opt-in ownership assertions when you want topology misuse to fail fast instead of staying purely contractual.
- `arc::Mpsc` gives bounded lock-free fan-in when several producers must feed one Core 0 consumer; `arc::Mux` is the terse topology alias.
- `arc::DenseMpsc` keeps the same algorithm but packs cells tightly when RAM density matters more than cache-line isolation.
- `arc::Fanin` gives per-producer SPSC lanes and batch push/pop paths when producer preemption must not block completed work from other producers.
- `arc::SeqReg` gives multi-word latest-snapshot handoff without queues or torn reads.
- `arc::Rcu<T>` gives dual-buffer latest-config handoff for large immutable snapshots without reader retries.
- `arc::RpcLane` gives typed request/reply handoff over static SPSC lanes for lock-free Core 0 to Core 1 commands.
- `arc::dmabuf`, `arc::simdbuf`, `arc::ahbbuf`, `arc::axibuf`, and one-word STL allocators make DMA/SIMD/descriptor/RTC-capable heap placement explicit.
- `arc::Space` reports runtime flash, OTA slot, partition, and heap capacity without heap allocation.
- `arc::Ota` wraps staged OTA writes and slot state without raw handle plumbing.
- `arc::FlashLog` queues fixed records into a static lane and flushes them through a caller-provided storage sink.
- `arc::Store` gives typed NVS blobs and fixed-buffer strings without raw handle plumbing in user code.
- `arc::net::Radio` is the shared Core 0 Wi-Fi owner, so UDP and ESP-NOW do not each re-create NVS, netif, event loop, and Wi-Fi driver state.
- `arc::net::Csi`, `CsiRx`, and `EspWifiCsiPolicy` capture Wi-Fi CSI, push fixed RF frames through `arc::Spsc`, extract amplitude/phase features, and quantize them for `arc::ml::QuantDenseS8`.
- `arc::net::Ftm` wraps ESP-IDF FTM initiator sessions and solves X/Y/Z RF multilateration with fixed `arc::dsp::Matrix` storage.
- `arc::net::Eap` configures WPA2/WPA3 Enterprise credentials and joins through the shared radio without pulling WPA supplicant into non-enterprise builds.
- `arc::net::Tcp` gives direct IPv4/IPv6 TCP socket clients for Core 0 control/config paths.
- `arc::net::Poll` multiplexes caller-owned TCP/TLS sockets from one Core 0 task without hidden heap allocation or per-connection stacks.
- `arc::net::IpFamily` lets TCP calls and UDP policies stay dual-stack by default or force IPv4/IPv6 explicitly.
- `arc::net::Tls` gives direct ESP-TLS client sessions for secure caller-buffer transports such as MQTTS without taking over reconnect or protocol policy.
- `arc::net::Pbuf` gives RAII ownership and direct payload spans for lwIP pbufs when a path needs packet-buffer ownership without extra copies.
- `arc::net::Http` gives RAII ownership for ESP-IDF HTTP/HTTPS client sessions, with explicit HTTPS factories that preserve secure URL enforcement across later URL resets.
- `arc::net::HttpServer` parses HTTP/1.x requests, trims fixed header/query views, emits small text/JSON responses, and routes compile-time method/path tags without allocation.
- `arc::pack::Schema`, `arc::pack::StructOf`, and `arc::pack::Reflect<T>` pack fixed binary telemetry/config records with compile-time field sizing, caller-owned buffers, and byteswap-based endian conversion.
- `arc::net::NetRpc` maps explicit struct codecs onto radio/transport payloads for zero-allocation distributed commands.
- `arc::net::Tdma` calculates deterministic ESP-NOW transmit windows from a synchronized microsecond clock.
- `arc::net::SwarmSchedule` assigns deterministic TDMA windows to known node IDs for collision-free fleet telemetry.
- `arc::net::DistributedRcu` sends RCU snapshots as fixed ESP-NOW-friendly frames, rejects stale/out-of-slot updates, and pairs with `DeadReckoning` for Kalman prediction after missed packets.
- `arc::Crdt<State, Peers>` adds fixed-size G-Counter, PN-Counter, and LWW-register state convergence for swarm partitions without heap allocation or a master node.
- `arc::Bft<T, Peers>` collects bounded `2f + 1` vote certificates for critical fleet decisions while rejecting same-node equivocation.
- `arc::net::Fabric` adds static next-hop routing over deterministic ESP-NOW swarm slots without dynamic discovery or heap allocation.
- `arc::net::EthernetRing` stores raw Layer 2 frames in a fixed ring for SPI/RMII Ethernet policies that bypass lwIP.
- `arc::net::W5500Raw` is a policy-driven SPI Ethernet path for raw frame pipelines without lwIP ownership.
- `arc::net::Mqtt` gives caller-buffer MQTT 3.1.1 packet encoding, parsing, topic matching, and heapless session keepalive helpers without owning the transport lane.
- `arc::net::Ws` gives WebSocket handshake helpers plus caller-buffer frame encode/parse without owning reconnect or task policy.
- `arc::net::Coap` gives caller-buffer CoAP datagram encode/parse and option walking without hiding message layout.
- `arc::net::Stream`, `Rtp`, and `Mjpeg` frame caller-owned byte streams for binary records, RTP media payloads, and multipart JPEG output.
- `arc::hls` marks fixed-bound kernels, builds `SiliconPlan` metadata, and provides unrolled FIR/dot helpers so DSP/ML/control code can stay compatible with HLS-style synthesis flows.
- `arc::net::Mdns` adds a thin discovery battery on top of `esp_netif` and the shared Wi-Fi lane when lwIP mDNS headers exist.
- `arc::Ble` gives a NimBLE lifecycle, host-task, GAP, advertising, and scanning bridge without taking over GATT profile design.
- `arc::net::Udp` is a reusable IPv4/IPv6 Core 0 transport plane when you opt into `#include "arc/udp.hpp"`.
- `arc::net::EspNow` is a reusable Core 0 raw-radio plane when you opt into `#include "arc/espnow.hpp"`.
- `arc::usb::DeviceDescriptor`, `Device`, `Bulk`, `Uvc`, `Uac`, and `Fifo` cover compile-time USB descriptors, Chapter 9 setup flow, class setup dispatch, and FIFO lane bridges.
- `arc::usb::Host` is a lean policy facade for USB host bring-up, polling, and transfers when a board owns the class stack.
- `arc::nav::Eskf`, `arc::nav::Quaternion`, `arc::ml::Snn`, and `arc::MagLev` cover high-rate inertial fusion, spike-driven inference, and unstable state-space control surfaces.

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
| MCPWM/LEDC/RMT/PCNT/SDM | native typed wrappers plus quadrature helpers | native drivers | mixed wrappers/libraries | components/limited |
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
- `tools/framework-compare.sh` reports local raw ESP-IDF and Arduino-ESP32 versions, runs or explicitly skips the Arc host benchmark depending on the caller, and compiles real host-buildable Arduino `Print.cpp` plus ESP-IDF mbedTLS paths for same-binary write/frame/base64 measurements with explicit `arc`, `idf`, and `arduino` surface tags.
- `examples/esp32s3/bench` is the real ESP32-S3 benchmark firmware. It flashes to hardware, reports grouped target cycle-counter measurements with an explicit `surface` column (`arc`, `idf`, `arduino`, or `std`), includes critical usage mixes for telemetry, control ticks, and protocol bundles, compares Arc against raw ESP-IDF silicon APIs in the same firmware image, covers internal temp/NVS/OTA plus self-test TWAI loopback in one run, and can add Arduino-ESP32 core write/base64 baselines when `arduino-esp32/` or `ARC_ARDUINO_PATH` is present during configure.
- Raw ESP-IDF benchmarks must live as pinned ESP-IDF examples or components and report firmware size, build config, target, and measured hardware path.
- Arduino or PlatformIO benchmarks must pin the core/platform version and board package in CI before any number is published.
- No README number should compare against Arduino, ESPHome, PlatformIO, or raw ESP-IDF unless that exact competitor source is checked in or installed by CI and run in the same workflow.

Reference docs: [ESP-IDF ESP32-S3 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/), [Arduino-ESP32 documentation](https://docs.espressif.com/projects/arduino-esp32/), [Arduino as an ESP-IDF component](https://docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html), [PlatformIO ESP-IDF framework docs](https://docs.platformio.org/en/latest/frameworks/espidf.html), and [ESPHome ESP32 platform docs](https://esphome.io/components/esp32.html).

Arc project docs live in `docs/` and are also built as a VitePress default-theme GitHub Pages site. The README is the full single-file manual; the website splits the same material into a reading path: `docs/getting-started.md`, `docs/architecture.md`, `docs/safety-patterns.md`, `docs/production-integration.md`, `docs/troubleshooting.md`, `docs/glossary.md`, `docs/modules.md`, `docs/module-pages.md`, `docs/api.md`, `docs/examples.md`, `docs/licensing.md`, `docs/benchmarking.md`, `docs/safety-case.md`, `docs/hil-test-jig.md`, and `docs/references.md`. `docs/safety-patterns.md` gives the copyable ownership, loan-pack, core-handoff, proof-pack, and validation shapes for first-time Arc code. `docs/production-integration.md` carries the product integration checklist for CMake shape, target policy, board topology, validation level, release evidence, and license duties. `docs/troubleshooting.md` maps common setup, build, editor, docs-site, benchmark, and evidence failures to concrete fixes. `docs/glossary.md` keeps Arc vocabulary consistent across the README, website, and generated module pages. `docs/modules.md` maps every public header to the problem it solves, `docs/module-pages.md` indexes one generated page per public header, and `docs/api.md` mirrors this README's API reference so the web docs do not stop at a high-level overview. `tools/docs-module-pages.py` regenerates the per-header pages from the real header list and module guide. `tools/safety-case-check.py` keeps the safety evidence map source-backed, prevents accidental certification overclaims in docs, and can emit JSON release evidence with `--format json`.

Beginner reading path:

1. Read `docs/getting-started.md` for setup and the shortest build loop.
2. Read `docs/architecture.md` to understand why Core 0 and Core 1 have different jobs.
3. Read `docs/safety-patterns.md` before writing shared state, cross-core handoff, or proof metadata.
4. Read `docs/production-integration.md` before moving example code into a product tree.
5. Open `docs/troubleshooting.md` when setup, CMake, editor, docs-site, benchmark, or evidence checks fail.
6. Use `docs/glossary.md` to decode Arc-specific terms.
7. Use `docs/modules.md` to choose the right Arc header.
8. Use `docs/module-pages.md` for a page dedicated to that header.
9. Use `docs/examples.md` to pick a working firmware project near that hardware lane.
10. Use `docs/api.md` for exact public methods, failure behavior, and ownership notes.

Host tooling: `tests/host/fuzz_codecs.cpp` is a default-compiled smoke target and opt-in libFuzzer harness for HTTP, URI, MQTT, WebSocket, and CoAP parsers. `tools/arc-pack-bridge.py` decodes fixed `arc::pack` frames into JSON/Foxglove-style JSONL, `tools/arc-gen.go` extracts `ARC_PACK_REFLECT` schemas for TypeScript and Go bridge code, `tools/compile-cost.py` ranks Clang `-ftime-trace` JSON so template-heavy headers can be optimized from evidence, `tools/arc-audit.go -all` scans every local `loop`/`step` realtime entry call graph, `tools/topology-check.py` scans literal `arc::Pins<...>` packs for duplicate/out-of-range GPIOs, understands integer and `GPIO_NUM_*` tokens, can fail unresolved tokens with `--strict-unresolved`, and can print report, DOT, Mermaid graph, or JSON evidence output, `tools/compile-fail-check.py` keeps negative static-borrow contracts failing, `tools/use-after-move-check.sh` runs `clang-tidy`'s moved-from-use lint when available, `tools/hil-evidence-check.py` validates captured physical HIL JSONL artifacts, and `tools/arc-prove.sh` validates the SPSC, role-exposure, and consensus TLA+ specs before CI builds. `tools/bridge/main.go` listens for UDP telemetry, republishes decoded frames over a dependency-free WebSocket bridge, emits Perfetto power counters, and can chunk PIE hotpatch plans into Fabric/RDMA JSON for physical CI orchestration.

<details>
<summary>Quick API Map</summary>

| Area | Headers | Primary types |
| --- | --- | --- |
| Profile umbrellas | `arc/core.hpp`, `arc/memory.hpp`, `arc/net_codecs.hpp`, `arc/math.hpp`, `arc.hpp` | Subset entry points for substrate, coherency/DMA, no-heap codecs, DSP/math, and the compatibility umbrella |
| Core plane | `arc/task.hpp`, `arc/borrow.hpp`, `arc/coro.hpp`, `arc/bare_core.hpp`, `arc/intermittent.hpp`, `arc/stack.hpp`, `arc/plane.hpp`, `arc/sketch.hpp`, `arc/tight.hpp`, `arc/lockstep.hpp`, `arc/sim.hpp`, `arc/proof.hpp`, `arc/rtos.hpp`, `arc/fsm.hpp`, `arc/flow.hpp`, `arc/ipc.hpp`, `arc/cli.hpp`, `arc/text.hpp` | `arc::spawn`, `arc::TaskMem`, `arc::Task`, `arc::TaskArena`, `arc::CoreLocal`, `arc::CoreMsg`, `arc::StaticRef`, `arc::StaticLoan`, `arc::StaticEdit`, `arc::LoanPack`, `arc::proof::Pack`, `arc::proof::Deadline`, `arc::BareCore`, `arc::power::Intermittent`, `arc::stack`, `arc::Plane`, `arc::App`, `arc::Tight`, `arc::Lockstep`, `arc::LockstepFault`, `arc::sim::Drive`, `arc::sim::Sense`, `arc::sim::Spi`, `arc::sim::Fifo`, `arc::sim::TraceLog`, `arc::sim::Harness`, `arc::rtos`, `arc::fsm::Automaton`, `arc::Flow`, `arc::Ipc`, `arc::Cli`, `arc::Command`, `arc::Text` |
| Ownership and topology | `arc/topology.hpp`, `arc/claim.hpp`, `arc/init.hpp`, `arc/audit.hpp`, `arc/roles.hpp` | `arc::Pins`, `arc::Topology`, `arc::Claim`, `arc::Gate`, `arc::TryGate`, `arc::MutexGate`, `arc::TryMutexGate`, `arc::Init`, `arc::InitTxn`, `arc::RefInit`, `arc::RefInitTxn`, `arc::RefLease`, `arc::Audit`, `arc::Roles` |
| Memory and coherency | `arc/caps.hpp`, `arc/cache.hpp`, `arc/cache_lock.hpp`, `arc/hotpatch.hpp`, `arc/copy.hpp`, `arc/dma_chain.hpp`, `arc/axi_graph.hpp`, `arc/pipeline.hpp`, `arc/mmu_span.hpp`, `arc/distributed_mmu.hpp`, `arc/fram.hpp`, `arc/place.hpp`, `arc/prefetch.hpp`, `arc/scrub.hpp` | `arc::dmabuf`, `arc::simdbuf`, `arc::Cache`, `arc::CacheLock`, `arc::HotPatch`, `arc::HotPatchImage`, `arc::HotPatchDetour`, `arc::DmaChain`, `arc::DmaEndpoint`, `arc::AxiGraph`, `arc::AxiPort`, `arc::AxiEdge`, `arc::Pipeline`, `arc::Dma2dWindow`, `arc::bind_rows`, `arc::MmuSpan`, `arc::mmu::DistributedSpan`, `arc::mmu::DistributedPager`, `arc::FramArena`, `arc::FramRef`, `arc::FramAlloc`, `arc::Copy`, `arc::prefetch`, `arc::Scrub` |
| Lock-free lanes | `arc/spsc.hpp`, `arc/mpsc.hpp`, `arc/fanin.hpp`, `arc/reg.hpp`, `arc/seq.hpp`, `arc/log.hpp`, `arc/postmortem.hpp`, `arc/rpc.hpp`, `arc/rtc_ring.hpp` | `arc::Spsc`, `arc::Mpsc`, `arc::DenseMpsc`, `arc::Fanin`, `arc::Reg`, `arc::SeqReg`, `arc::LogLane`, `arc::Postmortem`, `arc::RpcLane`, `arc::RtcRing` |
| GPIO and timing | `arc/drive.hpp`, `arc/sense.hpp`, `arc/gpio.hpp`, `arc/flexroute.hpp`, `arc/rtc.hpp`, `arc/timer.hpp`, `arc/etm.hpp`, `arc/time.hpp`, `arc/clock.hpp`, `arc/probe.hpp`, `arc/power_governor.hpp`, `arc/power_profiler.hpp`, `arc/timesync.hpp`, `arc/tdma.hpp`, `arc/covert.hpp`, `arc/lifi.hpp`, `arc/sdr.hpp` | `arc::Drive`, `arc::Sense`, `arc::Gpio`, `arc::matrix::FlexRoute`, `arc::RtcGpio`, `arc::RtcPin`, `arc::Timer`, `arc::Etm`, `arc::EtmRoute`, `arc::Time`, `arc::Clock`, `arc::Probe`, `arc::CycleStats`, `arc::JitterStats`, `arc::DeadlineStats`, `arc::StallStats`, `arc::power::Governor`, `arc::power::Profiler`, `arc::TimeSync`, `arc::net::Tdma`, `arc::covert::Fsk`, `arc::covert::EmTx`, `arc::covert::SonicTx`, `arc::optical::LiFi`, `arc::sdr::PulseSynth`, `arc::sdr::Tx` |
| Buses and data plane | `arc/any.hpp`, `arc/i2c.hpp`, `arc/spi.hpp`, `arc/i2s.hpp`, `arc/uart.hpp`, `arc/usb.hpp`, `arc/i80.hpp`, `arc/dvp.hpp`, `arc/sdio_slave.hpp`, `arc/pru.hpp` | `arc::AnyOut`, `arc::AnyIn`, `arc::AnyI2c`, `arc::AnySpi`, `arc::AnyUart`, `arc::I2cBus`, `arc::SpiBus`, `arc::I2s`, `arc::Uart`, `arc::Usb`, `arc::I80`, `arc::Dvp`, `arc::SdioSlave`, `arc::PruOut`, `arc::PruIn`, `arc::PruTiming`, `arc::PruCursor` |
| Storage and update | `arc/fs.hpp`, `arc/file.hpp`, `arc/sd.hpp`, `arc/store.hpp`, `arc/ota.hpp`, `arc/space.hpp`, `arc/flash_log.hpp`, `arc/secure_update.hpp` | `arc::Fs`, `arc::File`, `arc::Sd`, `arc::Store`, `arc::Ota`, `arc::Space`, `arc::FlashLog`, `arc::SecureUpdate` |
| Network and radio | `arc/net.hpp`, `arc/csi.hpp`, `arc/ftm.hpp`, `arc/sdr.hpp`, `arc/acoustic_slam.hpp`, `arc/hyper_matrix.hpp`, `arc/udp.hpp`, `arc/espnow.hpp`, `arc/fabric.hpp`, `arc/crdt.hpp`, `arc/bft.hpp`, `arc/rdma.hpp`, `arc/thread.hpp`, `arc/ble_mesh.hpp`, `arc/uri.hpp`, `arc/tcp.hpp`, `arc/poll.hpp`, `arc/pbuf.hpp`, `arc/tls.hpp`, `arc/http.hpp`, `arc/http_server.hpp`, `arc/mqtt.hpp`, `arc/ws.hpp`, `arc/coap.hpp`, `arc/xrce.hpp`, `arc/mdns.hpp`, `arc/eap.hpp`, `arc/netrpc.hpp`, `arc/swarm.hpp`, `arc/ethernet.hpp`, `arc/tsn.hpp`, `arc/w5500.hpp` | `arc::net::Radio`, `arc::net::Csi`, `arc::net::CsiRx`, `arc::net::EspWifiCsiPolicy`, `arc::net::Ftm`, `arc::net::EspWifiFtmPolicy`, `arc::sdr::Tx`, `arc::swarm::AcousticSlam`, `arc::swarm::HyperMatrix`, `arc::net::Udp`, `arc::net::EspNow`, `arc::net::Fabric`, `arc::Crdt`, `arc::GCounter`, `arc::PnCounter`, `arc::LwwReg`, `arc::Bft`, `arc::BftVote`, `arc::BftCert`, `arc::net::Rdma`, `arc::net::Thread`, `arc::ble::Mesh`, `arc::net::Uri`, `arc::net::UriView`, `arc::net::Tcp`, `arc::net::Poll`, `arc::net::Pbuf`, `arc::net::Tls`, `arc::net::Http`, `arc::net::HttpServer`, `arc::net::Mqtt`, `arc::net::Ws`, `arc::net::Coap`, `arc::xrce::Writer`, `arc::net::Mdns`, `arc::net::Eap`, `arc::net::NetRpc`, `arc::net::SwarmSchedule`, `arc::net::DistributedRcu`, `arc::net::DeadReckoning`, `arc::net::EthernetRing`, `arc::net::TsnSchedule`, `arc::net::TsnGate`, `arc::net::W5500Raw` |
| Stream utilities | `arc/stream.hpp` | `arc::net::Stream`, `arc::net::ByteStream`, `arc::net::AnyStream`, `arc::net::Rtp`, `arc::net::Mjpeg` |
| Binary records and optimizer hints | `arc/pack.hpp`, `arc/perfetto.hpp`, `arc/mcap.hpp`, `arc/trace_live.hpp`, `arc/assume.hpp` | `arc::pack::Schema`, `arc::pack::StructOf`, `arc::pack::Reflect`, `arc::pack::Endian`, `arc::PerfettoWriter`, `arc::mcap::Writer`, `arc::trace::LiveStream`, `arc::assume` |
| Control, ML, and vision | `arc/dsp.hpp`, `arc/wavefront.hpp`, `arc/simd.hpp`, `arc/ml.hpp`, `arc/snn.hpp`, `arc/matrix.hpp`, `arc/kalman.hpp`, `arc/nav.hpp`, `arc/foc.hpp`, `arc/maglev.hpp`, `arc/digital_twin.hpp`, `arc/motion.hpp`, `arc/cnc.hpp`, `arc/hls.hpp`, `arc/isp.hpp`, `arc/vision.hpp`, `arc/vision_accel.hpp`, `arc/vslam.hpp`, `arc/star_tracker.hpp`, `arc/ecs.hpp`, `arc/hil.hpp` | `arc::dsp::clarke`, `arc::dsp::park`, `arc::dsp::duty_svpwm`, `arc::dsp::DspAccel`, `arc::dsp::Beamform`, `arc::dsp::Aec`, `arc::dsp::Wavefront`, `arc::simd::float32x4_t`, `arc::simd::int8x16_t`, `arc::simd::dot_s8`, `arc::simd::Rgb565::from_yuv422`, `arc::simd::fft_radix2`, `arc::ml::Tensor`, `arc::ml::Dense`, `arc::ml::QuantDenseS8`, `arc::ml::Snn`, `arc::ml::Conv2dS8`, `arc::ml::DepthwiseConv2dS8`, `arc::ml::MaxPool2d`, `arc::ml::mapped_weights`, `arc::ml::Core1Inference`, `arc::dsp::Matrix`, `arc::dsp::Lqr`, `arc::dsp::Lqr::Adapted`, `arc::dsp::Kalman`, `arc::nav::Eskf`, `arc::nav::Quaternion`, `arc::Foc`, `arc::DualFoc`, `arc::FocEncoderFusion`, `arc::MagLev`, `arc::hil::DigitalTwin`, `arc::MotionPlan`, `arc::SCurve`, `arc::cnc::Kinematics`, `arc::cnc::GCode`, `arc::hls::KernelSpec`, `arc::hls::SiliconPlan`, `arc::hls::StaticLoop`, `arc::isp::Debayer`, `arc::isp::AecAwb`, `arc::vision::Sobel`, `arc::vision::OpticalFlow`, `arc::vision::PpaSrm`, `arc::vision::JpegEncoder`, `arc::vision::H264Encoder`, `arc::vision::VSlam`, `arc::vision::VisualServo`, `arc::vision::StarTracker`, `arc::SwarmSoa`, `arc::Hil` |
| USB and low-power logic | `arc/usb.hpp`, `arc/usb_device.hpp`, `arc/usb_host.hpp`, `arc/ulp.hpp`, `arc/ulp_asm.hpp`, `arc/ulp_cxx.hpp`, `arc/ulp_ml.hpp`, `arc/lp_core.hpp` | `arc::Usb`, `arc::usb::Device`, `arc::usb::DeviceDescriptor`, `arc::usb::Cdc`, `arc::usb::Bulk`, `arc::usb::Uvc`, `arc::usb::Uac`, `arc::usb::Fifo`, `arc::usb::Host`, `arc::Ulp`, `arc::ulp::riscv::assemble`, `arc::ulp::Builder`, `arc::ulp::Program`, `arc::ulp::Gpio`, `arc::ulp::Adc`, `arc::ulp::I2c`, `arc::ulp::SleepFsm`, `arc::ulp::ml::QuantDenseS8`, `arc::ulp::ml::SemanticWake`, `arc::ulp::ml::AudioSignatureWake`, `arc::lp::Image`, `arc::lp::Control`, `arc::lp::Mailbox` |
| Security, VM, and silicon | `arc/aes.hpp`, `arc/sha.hpp`, `arc/puf.hpp`, `arc/cloak.hpp`, `arc/blackbox.hpp`, `arc/cert_bundle.hpp`, `arc/nvs_crypto.hpp`, `arc/secure_boot.hpp`, `arc/hmac.hpp`, `arc/sign.hpp`, `arc/mpi.hpp`, `arc/kyber.hpp`, `arc/paillier.hpp`, `arc/xts.hpp`, `arc/fuse.hpp`, `arc/rng.hpp`, `arc/pms.hpp`, `arc/tee.hpp`, `arc/vm.hpp`, `arc/jit.hpp`, `arc/hotswap.hpp`, `arc/wasm_aot.hpp`, `arc/migrator.hpp`, `arc/hypervisor.hpp`, `arc/chaos.hpp`, `arc/flash_off.hpp`, `arc/crypto_dma.hpp`, `arc/interrupt_matrix.hpp`, `arc/provisioning.hpp`, `arc/pmr.hpp` | `arc::Aes`, `arc::Gcm`, `arc::Sha`, `arc::crypto::Puf`, `arc::crypto::Cloak`, `arc::covert::BlackBox`, `arc::x509::Bundle`, `arc::NvsCrypto`, `arc::secure::SecureBoot`, `arc::Hmac`, `arc::Sign`, `arc::Mpi`, `arc::crypto::Kyber`, `arc::crypto::Paillier`, `arc::Xts`, `arc::Fuse`, `arc::Rng`, `arc::Pms`, `arc::WorldGuard`, `arc::TeePlan`, `arc::vm::BPF`, `arc::vm::BpfInsn`, `arc::vm::BpfSandbox`, `arc::vm::Jit`, `arc::vm::HotSwap`, `arc::vm::HotSwapPlan`, `arc::vm::HotSwapGate`, `arc::vm::WasmAot`, `arc::vm::WasmSandbox`, `arc::swarm::AnyIdleCore`, `arc::swarm::Migrator`, `arc::vm::Hypervisor`, `arc::chaos::Monkey`, `arc::FlashOff`, `arc::CryptoDma`, `arc::InterruptMatrix`, `arc::Provisioning`, `arc::PmrCapsResource` |

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
│   ├── esp32s3/*          # stable ESP32-S3 firmware examples
│   ├── esp32s31/*         # experimental ESP32-S31 firmware examples
│   └── portable/*         # host-buildable/portable examples
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
│           ├── arc.hpp             # lean umbrella
│           └── arc/*.hpp           # feature headers
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
- Lockstep output comparison hooks for redundant safety loops
- Dedicated GPIO output and input on the hot path
- Hardware pulse counting through PCNT
- Hardware symbol streaming through RMT
- Hardware symbol capture through RMT RX
- Hardware MCPWM waveform generation with runtime frequency and duty retuning
- Hardware complementary MCPWM pairs with explicit dead-time and brake/fault shutdown
- Hardware MCPWM edge capture for period/high/low measurement
- GPIO matrix self-healing plans that move failed output/input signals to spare pads through board policy hooks
- Hardware ADC streaming through the digital controller and DMA
- Hardware async memory copy through the ESP32-S3 DMA memcpy path
- Static DMA endpoint, AXI graph, and pipeline composition for multi-stage descriptor graphs and 2D frame-window row binding
- Hardware LCD_CAM DVP camera input through DMA-backed frame buffers
- Hardware LCD_CAM Intel 8080 parallel output through DMA-backed panel IO
- PRU-style LCD_CAM/I2S descriptor rings for arbitrary protocol synthesis and parallel capture buffers
- Hardware I2C master/slave bus/device binding for sensors, EEPROMs, displays, control chips, and peer controllers
- Hardware SPI master/slave transfers with explicit bus/device composition and DMA-capable paths
- Hardware I2S streaming with duplex DMA and event counters
- 16+ channel acoustic wavefront phase planning and interleaved synthesis for TDM playback
- Hardware UART serial lanes for GPS, modems, consoles, and legacy links
- Hardware USB Serial/JTAG byte IO for cabled control channels
- Hardware USB OTG PHY ownership for native host/device stack bring-up
- Compile-time USB device descriptors and FIFO lane bridges for external class-stack integration
- Hardware SD/MMC FAT mounting and raw sector access
- Hardware PWM offload through LEDC for periodic output that should not burn a core
- Hardware Sigma-Delta pulse-density output with IRAM-safe density updates enabled
- Hardware timebase and alarms through GPTimer
- Deep-sleep and light-sleep entry with explicit wake-source and power-domain policy
- Hardware-backed AES, GCM, SHA, HMAC, Digital Signature, and XTS flash-encryption paths through Espressif's crypto drivers
- PUF-style SRAM/ADC entropy capture with Von Neumann extraction and SHA-256 key derivation hooks
- RTC no-init dying-gasp checkpoints for intermittent, batteryless execution policies
- ULP RISC-V and FSM load/run/control hooks for low-power coprocessor work
- Policy-based ULP C++ GPIO/I2C/ADC/SleepFsm helpers for tiny always-on sensing loops
- ULP-side int8 dense inference over I2C sensor samples for semantic wake decisions before the main cores power up
- ULP-side audio signature binning for ADC/PDM wake classification
- Hardware die-temperature telemetry for thermal guard logic
- Hardware capacitive touch sensing with typed controller/channel ownership, filter hooks, and sleep-wakeup hooks
- Lock-free SPSC/MPSC queues and single-word control register
- Wi-Fi CSI capture, RF amplitude/phase feature extraction, and fixed-point presence classifier input preparation
- LCD_CAM-oriented 1-bit SDR pulse synthesis for AM/FM/OOK experiments from caller-owned audio or bit spans
- ML-KEM-shaped Kyber keypair/encapsulation/decapsulation, SIMD-assisted pointwise products, NTT round-trips, and zero-allocation buffers
- Distributed RCU snapshots, TDMA slot enforcement, and Kalman dead-reckoning hooks for ESP-NOW fleets
- Heapless CRDT counters and LWW registers for partition-tolerant fleet state convergence
- Bounded BFT vote certificates for fleet abort/commit decisions with same-node equivocation rejection
- Acoustic FMCW chirp generation, TDoA extraction, 3D trilateration, and Kalman correction for synchronized swarms
- Camera-frame star thresholding, sub-pixel centroid extraction, and compressed-catalog triangle matching
- PMS/TEE-backed micro-hypervisor partition planning, trap binding, and explicit emulation/termination decisions
- Bounded chaos injection for SRAM flips, I2C stalls, ESP-NOW packet drops, and Tight-loop budget overruns with Postmortem logging
- Static deterministic mesh next-hop routing over TDMA ESP-NOW swarm slots
- Side-channel cloak hooks for randomized pipeline stalls, dummy reads, and inverse-load power flattening
- Error-state inertial fusion, spike-based neural layers, and MagLev state-space control surfaces
- Aligned RDMA frame planning, Manchester Li-Fi symbol generation, BPF JIT image emission, BlackBox record sealing, and digital-twin plant stepping
- Heapless CLI dispatch, SDIO slave DMA ownership, USB Chapter 9 state transitions, Thread/BLE Mesh policy bridges, IPC emergency hooks, and security-state wrappers
- Paillier encrypted telemetry aggregation over `Mpi`-style modular exponentiation/multiplication backends
- PWM/Sigma-Delta FSK planning for EM and ultrasonic air-gap signaling experiments
- Executable-payload loading and policy-controlled IRAM detour installation under level-15 interrupt masking
- Slot-aware OTA helper for staged writes and rollback state
- Typed NVS persistence and bounded string loading on the Core 0 side
- SPIFFS and FAT-on-flash VFS mounting
- RAII file handles for mounted VFS/FAT/SPIFFS/LittleFS paths
- Direct TCP clients for small control/config protocols
- RAII HTTP client sessions for REST/config exchanges
- BLE 5 host lifecycle, GAP naming, advertising, and scanning through NimBLE
- Bounded MPSC fan-in for multi-task telemetry producers
- SIMD-friendly float, int8, RGB/YUV, FFT, and ML kernels that fit the Core 1 compute plane
- Acoustic beamforming, FFT/GCC-PHAT lag estimation, and NLMS acoustic echo cancellation over caller-owned sample buffers
- Caller-buffer ISP and vision kernels for raw Bayer frames, RGB565 statistics, edge detection, optical flow, and visual-servo targets
- Zero-allocation BPF-style bytecode execution for hot-swappable control logic loaded from fixed binary blocks
- Cycle-counter instrumentation for hot-path measurement

## C++ Reality

Arc is pinned to the ESP-IDF v6.0 patch line and should be validated with the current repo pin:

- ESP-IDF `v6.0.1`
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
- `components/arc/profiles.json` is the source-of-truth profile manifest, and `tools/profile-manifest-check.py` keeps it synchronized with `cmake/arc-deps.cmake` while preventing substrate profiles from importing domain profile roots.
- `tools/profile-export.py` can materialize a profile header closure such as `core`, `memory`, `net_codecs`, `math`, `crypto`, `robotics`, or `sandbox` into `dump/profiles/<profile>` or an external component directory with a header-only ESP-IDF component stub and the profile's ESP-IDF `REQUIRES`. Existing output is refreshed only when it is empty or marked as an Arc profile export.

Use this pattern in `main/CMakeLists.txt` or any example `main/CMakeLists.txt`:

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core timer)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

Export a standalone profile package when another project only needs one Arc subset:

```bash
./tools/profile-export.py core
```

Feature names map directly to hardware lanes:

Profile aliases for `math`, `memory`, `net_codecs`, `crypto`, `robotics`, and `sandbox` are listed here too so CMake declarations can match the profile headers directly.

- `acoustic_slam`
- `gpio`
- `adc`
- `aes`
- `ble`
- `ble_mesh`
- `borrow`
- `blackbox`
- `cert_bundle`
- `chaos`
- `cli`
- `cnc`
- `cloak`
- `covert`
- `copy`
- `crypto`
- `csi`
- `digital_twin`
- `distributed_mmu`
- `eap`
- `etm`
- `fabric`
- `flexroute`
- `flow`
- `fuse`
- `math`
- `memory`
- `mcap`
- `mcpwm`
- `rmt`
- `mpi`
- `pcnt`
- `sha`
- `sim`
- `sign`
- `ledc`
- `pwm` (`ledc` alias)
- `gptimer`
- `timer` (`gptimer` alias)
- `hmac`
- `hotpatch`
- `hotswap`
- `hls`
- `http`
- `hyper_matrix`
- `mdns`
- `nav`
- `nvs_crypto`
- `i2c`
- `i2s`
- `intermittent`
- `hypervisor`
- `ipc`
- `jit`
- `kyber`
- `lifi`
- `lockstep`
- `lp_core`
- `maglev`
- `spi`
- `sd`
- `twai`
- `can` (`twai` alias)
- `cam`
- `lcd`
- `sdm`
- `temp`
- `store`
- `net`
- `net_codecs`
- `ota`
- `otg`
- `paillier`
- `pm`
- `poll`
- `power_governor`
- `power_profiler`
- `proof`
- `puf`
- `rdma`
- `rng`
- `robotics`
- `rtc`
- `sandbox`
- `scrub`
- `sdr`
- `sdio_slave`
- `secure_boot`
- `space`
- `snn`
- `star_tracker`
- `time`
- `thread`
- `touch`
- `trace_live`
- `trax`
- `ftm`
- `fram`
- `fs`
- `migrator`
- `tcp`
- `tls`
- `tsn`
- `uri`
- `udp`
- `uart`
- `ulp`
- `ulp_ml`
- `usb`
- `usb_device`
- `vslam`
- `vision_accel`
- `usb_host`
- `wasm_aot`
- `wavefront`
- `wdt`
- `xts`
- `xrce`
- `espnow`

Add `gpio` when you use `arc::Drive`, `arc::Sense`, or raw `arc::Gpio`, because those pin APIs depend on the GPIO driver headers.
Add `rtc` when you use `arc::RtcGpio` or `arc::RtcPin`; it maps to the public RTC IO driver and should be used for sleep-retained pad state, pad isolation, and RTC wake plumbing.
Add `i2c` for both master (`arc::I2cBus`, `arc::I2c`) and slave (`arc::I2cSlave`) ownership; they share the same ESP-IDF I2C driver component and the same Arc port claim lane.
Add `spi` for both master (`arc::SpiBus`, `arc::Spi`) and slave (`arc::SpiSlave`) ownership; they share the same ESP-IDF SPI driver component and the same Arc host claim lane.

For the fastest compile times, keep each app honest: include only the Arc headers you use directly, and request only the matching Arc features in CMake. When a header-heavy change feels slow, compile a representative translation unit with Clang `-ftime-trace` and run `./tools/compile-cost.py build/path/to/file.json --top 20` to rank the template/include events by aggregate milliseconds before refactoring.

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

`arc::Pins` ignores negative sentinel pins like `-1` and ESP-IDF's `GPIO_NUM_NC`, so optional CS/MISO-style values can still be represented without false collisions.
Run `./tools/topology-check.py path/to/board.hpp` to get a host-side list for literal `arc::Pins<...>` packs before waiting on template diagnostics. The tool accepts plain integer literals, C++ base prefixes and digit separators, and ESP-IDF tokens such as `GPIO_NUM_4`; use `--strict-unresolved` when CI should reject tokens it cannot reduce. Use `--format report` for a grouped beginner-readable report, `--format dot` for Graphviz, `--format mermaid` for a Markdown-friendly graph, or `--format json` for release evidence tooling.

Peripheral wrappers also claim physical silicon at `init()` time. Two different `arc::Uart`, `arc::I2cBus`, `arc::I2cSlave`, `arc::I2c`, `arc::SpiBus`, `arc::SpiSlave`, or CS-backed `arc::Spi` template aliases cannot silently own the same hardware with different type parameters; the later claim returns `ESP_ERR_INVALID_STATE`.
I2C and UART wrappers also reject pre-existing raw ESP-IDF drivers by default, because Arc cannot prove their pin, buffer, or framing policy. Set the trailing `AdoptExisting` template parameter only when deliberate raw-driver interop is part of the board contract.
Identical aliases share a tiny atomic init gate, so two tasks racing the same first `init()` do not double-call the ESP-IDF allocator.

## Resource Lifetime

Arc keeps driver lifetime explicit because ESP-IDF peripheral handles often have process-wide side effects.

- `arc::Init` is the smallest boot-once state machine: `empty -> busy -> ready`, with `take()` giving exclusive teardown access. `arc::InitTxn` wraps first initialization so early returns roll back to `empty` instead of leaving a resource stuck in `busy`.
- `arc::RefInit` is the shared-resource variant. The first `begin()` owns initialization, later `begin()` or `take()` calls acquire a reference, and only the final `drop()` returns `true` so the caller can tear the hardware down. `arc::RefInitTxn` gives the first initializer the same rollback-on-early-return protection as `arc::InitTxn`.
- Both init machines expose `is_empty()`, `is_busy()`, and `is_ready()` so diagnostics can inspect state without depending on sentinel values.
- `arc::RefLease` is the scoped form for borrowed shared references. `take()` acquires a new reference, `adopt()` wraps an existing one, and the final owner must call `release()` and perform teardown explicitly instead of hiding hardware deinit in a destructor.
- `arc::StaticRef<&object, Owner>` is the compile-time form for static app state. `read<Core>()` returns a copyable readonly `StaticLoan`, `write<Core>()` returns a move-only mutable `StaticLoan`, and both disappear from the overload set for the wrong core owner or const storage. `arc::StaticEdit<WriteRef, ReadRefs...>` is the common one-writer task contract, while `arc::LoanPack<...>` is the fully explicit form: repeated readonly loans are allowed, but a mutable static loan cannot be packed beside another loan to the same object.
- `arc::Gate` is a tiny task-context guard for protecting short hot mutations. It asserts in ISR context instead of blocking where FreeRTOS cannot safely sleep.
- `arc::TryGate` is the non-blocking form for paths that may probe ownership but must not sleep; failed acquisition is just `false`.
- `arc::MutexGate` and `arc::TryMutexGate` wrap a lazily created `StaticSemaphore_t` mutex for task-level control-plane locks that should get FreeRTOS priority inheritance instead of spin-sleep polling.

This keeps static peripheral wrappers cheap while still giving buses, radios, filesystems, and host tasks a path to safe dynamic off/on behavior when an application needs sleep, hot-plug, or staged subsystem startup.

## License

Arc is licensed under `AGPL-3.0-only`, with the license notice in `LICENSE` and the full GNU Affero GPL v3 text in `COPYING`.

The intent is strict open source: you can study, use, modify, and redistribute Arc under AGPLv3-only, but downstream modifications must preserve the same reciprocal source-availability obligations.

Commercial/proprietary use outside AGPLv3-only is not granted for free. The paid commercial path is documented in `COMMERCIAL-LICENSE.md` and `docs/licensing.md`; it keeps Arc-covered portions visible, auditable, and bounded to the signed product scope while granting proprietary distribution rights only through a signed agreement.

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
git clone --branch v6.0.1 --recursive https://github.com/espressif/esp-idf.git
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
./tools/sync-idf.sh --stash --install
```

`sync-idf.sh` reads Arc's pinned `ARC_IDF_VERSION` and `ARC_IDF_REF` from `.github/workflows/build.yml`, stashes a dirty local `esp-idf/` only when `--stash` is passed, checks out the pinned release, and updates submodules. When Arc moves to the next ESP-IDF patch release, rerun the same command instead of hand-editing the local checkout.

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

It also exports `IDF_TARGET` from `ARC_TARGET`, defaulting to `esp32s3`. Every project CMake entry routes through `cmake/arc-idf.cmake`, which keeps ESP32-S3 as the stable default and rejects unknown targets instead of silently falling back.

## Target Selection

Arc keeps ESP32-S3 as the default while allowing explicit non-default targets. The selector is:

- `ARC_TARGET=esp32s3` by default; this still forces `IDF_TARGET=esp32s3`.
- `ARC_TARGET=esp32p4` configures ESP32-P4 when the pinned ESP-IDF checkout provides that target.
- `ARC_TARGET=esp32s31` only configures when `ARC_EXPERIMENTAL_ESP32S31=ON` is also set.
- Any unknown `ARC_TARGET` fails during configure.

The root firmware and ESP32-S3 examples under `examples/esp32s3/` each declare `arc_target(esp32s3)`, so they fail fast instead of inheriting an incompatible target from the shell. Target-neutral examples live under `examples/portable/`. ESP32-S31 scaffolds live under `examples/esp32s31/`, declare `arc_target(esp32s31)`, and are intentionally skipped by default CI until ESP-IDF exposes a usable `esp32s31` target.

ESP32-P4 is Arc's high-performance no-radio target profile: dual-core RISC-V, 400 MHz class CPU clock, SIMD, Ethernet MAC/PTP, USB OTG, camera/display, MIPI CSI/DSI, JPEG, GDMA, and 2D-DMA facts are available through `arc::soc::Esp32P4` and `arc::soc::p4`. Radio-backed features still stay off because the chip has no Wi-Fi/BLE.

Example P4 configure flow:

```bash
export ARC_TARGET=esp32p4
. ./env.sh
idf.py build
```

Example S31 configure flow:

```bash
export ARC_TARGET=esp32s31
export ARC_EXPERIMENTAL_ESP32S31=ON
. ./env.sh
idf.py -C examples/esp32s31/ptp build
```

The current pinned local ESP-IDF checkout does not contain `esp32s31` target metadata, so S31 firmware builds remain blocked on preview SDK support. The scaffolds still define the intended Arc surface:

- `examples/esp32s31/ptp`
- `examples/esp32s31/ml`
- `examples/esp32s31/amp`
- `examples/esp32s31/cam`
- `examples/esp32s31/control`

Migration note: ESP32-S3 and ESP32-S31 share Arc's high-level programming model, including `arc::App`, `arc::Tight`, `arc::Cache`, `arc::DmaChain`, `arc::PtpClock`, ML helpers, and TEE planning types. They do not share low-level private register assumptions. ESP32-S3-only surfaces such as dedicated GPIO `arc::Drive`/`arc::Sense`, Xtensa interrupt masking, and TRAX are guarded so S31 code gets explicit compile-time diagnostics instead of fake S3 behavior.

The public target API follows Arc's short naming policy:

```cpp
static_assert(arc::soc::s3 || arc::soc::p4 || arc::soc::s31);
arc::soc::has<arc::soc::Cap::ptp>;
arc::CoreMap<>::det;
```

See `docs/api-naming.md` for the API naming rule used by repo checks.

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
go run tools/dump-source.go
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
- Exposes feature booleans such as `wifi`, `ble`, `ble5`, `ble_mesh`, `ble_privacy`, `usb_otg`, `usb_jtag`, `etm`, `simd`, `async_copy`, `ahb_dma`, `dma2d`, `ppa`, `jpeg`, `h264`, `fast_gpio`, `rtc_gpio`, `rtc_io`, `rtc_hold`, `rtc_wake`, `lcd_i80`, `dvp`, `aes_dma`, `sha512_256`, `hmac`, `sign`, `ecdsa`, `xts`, `ulp_fsm`, and `ulp_riscv`.
- Exposes hardware counts such as `gpio_pins`, `rtc_pins`, `adc_units`, `ledc_channels`, `spi_ports`, `rmt_words`, `uart_ports`, `sdmmc_slots`, `rsa_bits`, and `ds_bits`.
- `etm` and `ecdsa` are deliberately represented even when the pinned ESP32-S3 `soc_caps.h` does not advertise those driver surfaces.
- Contains hard `static_assert` guards for Arc's baseline contract: dual-core ESP32-S3, dedicated GPIO, async AHB-GDMA, and SIMD.

### `arc::Pins<...>` and `arc::Topology<Board>`

Compile-time board topology guard.

- `arc::Pins<...>` asserts that all non-negative physical pins in the pack are unique.
- `Pins::count`, `Pins::has<Pin>()`, and `Pins::index<Pin>()` keep board-owner assertions short without repeating the pin list.
- `arc::Topology<Board>` checks that `Board::pins` exists and passes the uniqueness rule.
- Use it in one central `Board`/`Hw` declaration, not scattered through application files.

This does not attempt impossible cross-translation-unit magic. It makes the intended hardware truth explicit and catches duplicate pins inside that truth source.

### `arc::Result<T>` and `arc::Status`

Opt-in C++23/26 runtime error surface based on `std::expected`.

- `arc::Result<T>` is `std::expected<T, esp_err_t>`.
- `arc::Status` is `std::expected<void, esp_err_t>`.
- `arc::fail(err)` creates the error side.
- `arc::status(err)` converts an `esp_err_t` into `arc::Status`.
- `arc::status_code(status)` converts `arc::Status` back to `esp_err_t` after monadic `.and_then(...)` chains.
- `ARC_TRY(name, expr)` unwraps a recoverable value expression or returns the same `esp_err_t` from the current function.
- `ARC_CHECK(expr)` propagates a recoverable status/error expression when the value is not needed.

Use this for runtime operations that can fail without invalidating the board topology. Hardware boot/configuration errors still intentionally use fail-fast ESP-IDF checks.

### `arc::Time`

Global microsecond clock backed by the ESP32-S3 SYSTIMER through `esp_timer_get_time()`.

- `us()` returns microseconds since boot.
- `ms()` returns milliseconds since boot.
- `next()` returns the nearest active esp-timer alarm timestamp.
- `wake()` returns the nearest alarm that should wake light sleep.
- `since(start)` and `due(start, span)` keep timeout math unsigned and terse.

Use this when Core 0 and Core 1 need one shared timestamp base. Keep `arc::Clock` for core-local cycle probes and short spin windows.

### `arc::rtos`

Small FreeRTOS time conversion helpers for APIs that accept wait windows.

- `milliseconds(duration)` rounds positive `std::chrono` durations up to milliseconds and saturates at `uint32_t` max.
- `ticks_ms(ms)` converts milliseconds to `TickType_t` without the overflow/truncation hazards of scattered `pdMS_TO_TICKS(...)` calls.
- `ticks(duration)` combines both conversions.
- `delay_for(duration)` sleeps the current task for the converted tick count.

`Uart`, `Usb`, and `AnyUart` keep their existing integer millisecond APIs and also accept `std::chrono` durations.

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

### `arc::vm::HotSwap`

Signed control-payload staging for native hotpatch, BPF JIT, and WASM AOT paths.

- `HotSwapPlan` carries the payload kind, version, payload bytes, signature bytes, and optional native entry offset.
- `HotSwapGate` is caller-owned anti-rollback state: `accept(plan)` rejects unsigned, unversioned, replayed, or downgraded plans, and `commit(image)` only marks the staged kind/version active after policy activation succeeds.
- `native<Policy, HeapPolicy>(plan)` verifies the plan and loads native executable bytes through `arc::HotPatch`.
- `bpf<Policy, JitPolicy>(plan, decoded, out, config)` verifies, decodes BPF bytes, translates them through `arc::vm::Jit`, and asks policy to protect the image.
- `wasm<Policy, AotPolicy>(plan, out, config)` verifies, translates WASM bytes through `arc::vm::WasmAot`, and asks policy to protect the image.
- `activate<Policy>(image)` is a separate explicit policy hook so Arc never jumps into downloaded code by default.

Use this when Core 0 has fetched a signed payload and the product needs one auditable path from verification to PMS/world protection to activation. Signature verification, key trust, executable-memory policy, persistent rollback storage, and jump policy remain product policy.

### `arc::swarm::AnyIdleCore` and `arc::swarm::Migrator`

Fixed fleet target selection and WASM process migration helpers.

- `FleetPeer` records one peer's node id, deadline slack, free cycle budget, overrun count, and online state.
- `AnyIdleCore::select(span, min_slack, min_cycles)` picks the online non-overrun peer with the best slack from a fixed-extent caller-owned span.
- `Migrator::from_governor(decision, idle_core)` converts a Core 0 governor decision plus selected peer into a `MigrationDecision`.
- `snapshot<MaxBytes>(process, state)` copies bounded WASM linear memory plus stack, instruction pointer, and fuel into a fixed `MigrationFrame`.
- `teleport<Policy>(peer, frame)` and `resume<Policy>(frame, memory)` delegate radio send and sandbox resume to board policy.

Use this when a fleet has measured peer state and a sandboxed Core 0 workload may be moved off-chip. Arc chooses only from explicit fixed state; peer discovery, trust, radio retries, and result return policy remain product code.

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
- `Ulp::Shared<T>` adds an 8-byte-aligned seqlock payload wrapper for trivially copyable RTC-shared structs.

Use this for always-on sensing, wake decisions, or low-power counters while the main cores sleep.

### `ARC_LP_CORE` and `arc::lp`

ESP32-P4 LP-core handoff contracts for builds that compile a small RV32IMC side image outside the host firmware TU.

- `ARC_LP_CORE`, `ARC_LP_DATA`, `ARC_LP_BSS`, and `ARC_LP_SHARED` place marked functions or objects in Arc LP-core sections.
- `Image{binary, load, entry, stack}` and `Control{image, period_us, wake_main}` describe a fixed LP payload and run policy.
- `load(policy, image)`, `start(policy, control)`, and `stop(policy)` validate the plan before calling board-owned hooks.
- `Word` is a 32-bit acquire/release shared word.
- `Mailbox<T>` provides two `arc::SeqReg<T>` lanes so host and LP code exchange multi-word snapshots without heap or locks.

Use this when P4 LP-core code is compiled by a board build rule but Arc should still own the C++ tags, fixed image metadata, and shared-state contract. Toolchain selection, linker scripts, LP-RAM placement, and wake-source wiring remain board policy.

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

- `arc::PlainPayload<T>` accepts copied values, stable IDs, fixed arrays, and standard/result wrappers that do not directly carry raw pointers, reference wrappers, spans, or string views.
- `arc::TrivialPayload<T>` adds the trivially-copyable requirement used by lock-free lanes and wire-style payloads.
- `arc::ControlResult<T>` accepts recoverable control functions that return `void`, `esp_err_t`, or `arc::Status`.
- `arc::DigitalOut<T>` and `arc::DigitalIn<T>` check the static GPIO-style surfaces used by `arc::Gpio`, `arc::Drive`, and `arc::Sense`.
- `arc::I2cDevice<T>`, `arc::SpiDevice<T>`, and `arc::UartDevice<T>` check static bus/device contracts for templated drivers that still want compile-time dispatch.
- `arc::WaveConfig<T>` checks that a static waveform type exposes `Config`, `defaults()`, `config()`, and `permille()`.
- `arc::ConfigWave<T>` checks the combined recoverable runtime-config surface.
- `arc::DutyWave<T>` checks the duty-control surface.
- `arc::RateWave<T>` checks the runtime frequency-control surface.

Use these when application code should accept "anything with this static control surface" instead of naming one exact Arc wrapper type.

### Internal Contracts

Most application code should only see the small surface: `start()`, `send(...)`, `read(...)`, `set(...)`, and typed buffers. The dense internals are deliberately isolated:

- 64-bit claim tokens protect hardware ownership across translation units while a tiny 32-bit gate avoids depending on target support for 64-bit atomics.
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

If `Program` declares `static constexpr std::size_t stack_bytes`, `App` refuses to compile unless `StackBytes >= stack_bytes`. Without an explicit declaration, Arc still enforces the 2 KiB `arc::stack::task_floor`.

`boot()` is idempotent while the task is active.

`arc::Sketch` remains available as a compatibility alias.

### `arc::Tight<Program, StackBytes, Core = core1, Guard = arc::Critical, Pri = max, Budget = 0>`

Masked per-step loop for the extreme path.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void step() noexcept`

`Tight` runs `step()` forever and constructs `Guard` around every iteration.

When `Budget` is non-zero, `Tight` counts iterations whose masked step exceeds that cycle budget. `overruns()` returns the counter, and an optional `Program::overrun(cycles) noexcept` hook can consume the exact overrun cost.

Like `App`, `Tight` enforces `Program::stack_bytes` at compile time when the program declares it, with `arc::stack::task_floor` as the minimum.

Use this only when the step body is tiny and the jitter budget is tighter than what a normal task iteration should tolerate. `arc::Hard` is a compatibility alias.

### `arc::stack`

Compile-time stack sizing helpers for Arc tasks.

- `arc::stack::task_floor` is the minimum accepted static task stack.
- `arc::stack::budget<LocalBytes, CalleeBytes, RuntimeBytes, MarginBytes>()` creates a rounded byte budget for `Program::stack_bytes` or `Policy::stack_bytes`.
- `arc::stack::storage<T, N>()` and `arc::stack::objects<T...>()` make local object budgets readable.
- `arc::TaskMem<StackBytes, RequiredBytes>` fails at compile time when `StackBytes` is smaller than `RequiredBytes`.
- `arc::Plane`, `arc::App`, and `arc::Tight` publish `stack_required` so tests can assert the expected budget.

Arc's component interface enables GCC `-Werror=stack-usage=2048` and `-Werror=frame-larger-than=1024` for C++ consumers by default. Set `ARC_ENFORCE_STACK_LIMITS=OFF` only for deliberate interop with components that cannot meet Arc's stack policy. The C++ interface also defaults consumers to `gnu++26` and `-mtext-section-literals`; set `ARC_ENFORCE_CONSUMER_CXX26=OFF` or `ARC_ENFORCE_CONSUMER_TEXT_LITERALS=OFF` only when a downstream component owns that compiler policy.

### `arc::CoreLocal<T, Core>` and `arc::CoreMsg<T, From, To>`

Compile-time core ownership tags for state that must not be casually shared across the AMP boundary.

- `CoreLocal<T, Core::core1>` stores the value privately and only exposes owner-gated access such as `snapshot()`, `set(value)`, `with(fn)`, and `msg<To>()`.
- `CoreLocalType<T>` and `CoreMsgType<T>` let templates require core-local slots or addressed core messages directly.
- `CoreLocal::can_access<Core>()` and `with(fn)` keep common owner checks and short scoped mutations readable without repeating the owner core.
- `CoreLocal::Msg<To>` and `CoreLocal::Incoming<From>` name outbound and inbound `CoreMsg` contracts without repeating the payload and owner core.
- `set<Core>(value)`, `with<Core>(fn)`, `msg<Core, To>()`, and `accept<Core>(msg)` remain available when a call site should spell the access core explicitly.
- Core-local state and core-message payloads must be copied values, stable IDs, or fixed arrays; direct pointers, reference wrappers, `std::span`, `std::string_view`, and standard/result wrappers containing those fail at compile time.
- `CoreLocal::with<Core>(fn)` and `CoreMsg::with<Core>(fn)` callbacks may return `void` or an ordinary value, but not a reference, raw pointer, `std::reference_wrapper`, common non-owning view such as `std::span` / `std::string_view`, or standard/result wrapper such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those.
- `snapshot()` copies the current value through the encoded owner core, avoiding a borrowed reference for simple reads.
- `msg<To>()` creates a copied message from the owner core to the destination core, and `accept(msg)` applies a message addressed to the local core.
- Access from the wrong core is absent from the overload set, so misuse fails during template checking instead of becoming a runtime convention.
- `CoreMsg<T, From, To>` carries a copied payload across a queue or mailbox type; `snapshot()` copies, `with(fn)` scopes a destination read, and `get()` borrows through the encoded destination core, while `with<To>(fn)` and `get<To>()` stay available for explicit boundary code.
- `accept<Owner>(msg)` applies an addressed message to the destination local slot.

Use this for small ownership-sensitive control or telemetry records where `Spsc`, `SeqReg`, or another lane carries the transfer and the value itself should still remember which core may touch it.

### `arc::StaticRef<&object, Owner>` and `arc::StaticLoan`

Compile-time static-lifetime loans for app state that would otherwise be passed
around as a raw pointer or reference.

- `StaticRef<&state, Core::core1>::read<Core::core1>()` returns a copyable readonly loan.
- `StaticRef<&state, Core::core1>::write<Core::core1>()` returns a non-copyable, non-movable mutable loan.
- `StaticRef::Read`, `StaticRef::Write`, `can_read<Core>()`, and `can_write<Core>()` keep common compile-contracts short enough for examples and static assertions.
- `StaticRef::Reads<ReadRefs...>` and `StaticRef::Edit<ReadRefs...>` name owner-anchored loan-pack contracts without repeating the writer owner type.
- `StaticRef::snapshot()` and `arc::snapshot<Ref>()` copy through the declared owner core, avoiding a borrowed reference for simple reads.
- `StaticRef::set(value)` and `arc::set<Ref>(value)` replace the whole static value through the declared owner core without exposing a mutable reference.
- `arc::snapshots<Refs...>()` copies several read-only static refs into a tuple through the inferred owner core.
- `StaticRef::snapshots<ReadRefs...>()` copies this owner plus other read-only refs through the inferred owner core.
- `StaticRef::snapshots<Core, ReadRefs...>()` keeps the explicit-core form for read-only copy-out packs.
- `StaticRef::with_read(fn)` and `StaticRef::with_write(fn)` scope a single static owner through its own declared core.
- `StaticRef::with_read<Core>(fn)` and `StaticRef::with_write<Core>(fn)` keep the explicit-core form for boundaries that should not infer from the owner.
- `StaticRef::with_reads<ReadRefs...>(fn)` scopes this owner plus other read-only refs without naming a separate `StaticReads` contract first.
- `StaticRef::with_reads<Core, ReadRefs...>(fn)` keeps the explicit-core form for mixed-boundary code.
- `StaticRef::with_edit<ReadRefs...>(fn)` scopes the common one-writer, many-reader edit without naming a separate `StaticEdit` contract first.
- `StaticRef::with_edit<Core, ReadRefs...>(fn)` keeps the explicit-core form for mixed-boundary code.
- Scoped borrow callbacks may return `void` or an ordinary value, but not a reference, raw pointer, `std::reference_wrapper`, common non-owning view such as `std::span` / `std::string_view`, standard/result wrapper such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those, or `StaticLoan`.
- `StaticReads<Refs...>` and `StaticWrites<Refs...>` build `LoanPack` contracts from `StaticRef` owner types instead of raw object addresses.
- `StaticEdit<WriteRef, ReadRefs...>` builds the common one-writer, many-reader `LoanPack` without manually naming each loan type.
- `LoanPack<Loans...>` rejects packs where one static object has a mutable loan plus any other loan.
- `LoanPack::contains<Loan>()`, `reads<Ref>()`, `reads<&object>()`, `writes<Ref>()`, and `writes<&object>()` make task-boundary requirements testable.
- `LoanPack::can_access<Core>()` checks whether the whole pack is usable from one core owner.
- `LoanPack::with<Core>(fn)` invokes a callback with the pack's readonly and mutable references in declared order, after the same core-owner and alias checks.
- `HasLoan`, `HasStaticRead`, and `HasStaticWrite` let function templates require a loan pack before accepting a task contract.
- `HasStaticRefRead` and `HasStaticRefWrite` are the same query shape when the contract should stay expressed in `StaticRef` owner types.
- `StaticReadable`, `StaticWritable`, `LoanReadable`, and `LoanWritable` let templates check whether a static owner or loan can be used from a specific core.
- `with_read<Ref, Core>(fn)`, `with_reads<Refs...>(fn)`, and `with_write<Ref, Core>(fn)` keep loans scoped to one callback when code should not hold them across a larger block.
- `with_reads<Core, Refs...>(fn)` keeps the explicit-core form available for read-only packs that should not infer from the first ref.
- `snapshots<Core, Refs...>()` keeps the explicit-core form available for read-only copy-out packs.
- `with_edit<WriteRef, ReadRefs...>(fn)` uses the writer's owner for the common one-writer, many-reader path.
- `with_edit<Core, WriteRef, ReadRefs...>(fn)` keeps the explicit-core form available when the boundary is not the writer owner.
- `loans_ok<Loans...>()` lets compile-contract tests check the same alias rule without intentionally failing the build.
- Access from the wrong core owner is absent from the overload set.
- Pointer shorthand through `operator->` and `operator*` is only available for `Core::any`; owner-bound loans must name the accessing core through `get<Core>()` or the scoped helpers.
- `write<...>()` is absent for const storage, and instantiating a mutable loan over const storage fails with an Arc diagnostic.

Use this when a task, simulator harness, or policy object should receive state
with its static storage and core owner visible in the C++ type. It is an Arc
boundary contract, not a whole-program C++ alias analysis engine.

### `arc::Link<Event, Control, Capacity>`

Shared state for asymmetric programs.

- `events` is an `arc::Spsc<Event, Capacity>`
- `pace` is an `arc::Reg<std::uint32_t>`
- `control` is an `arc::Reg<Control>`

Use it when Core 1 emits ordered events and Core 0 applies latest-wins control.

`arc::Bus` remains available as a compatibility alias.

### `arc::Flow<Source, Lane, Sink>`

Static source/lane/sink blueprint for simple data paths.

- `Source` declares `using value_type = ...` and `static bool read(value_type&) noexcept`.
- `Lane` is a static queue with matching `value_type` and producer/consumer endpoints, for example `arc::Spsc<T, N>` or `arc::Mpsc<T, N>`.
- `Sink` declares `static bool write(const value_type&) noexcept`.
- `fill()` reads at most one source item and queues it, retaining one pending item if the lane is full.
- `drain()` moves one queued item to the sink, retaining the popped item if the sink is temporarily full, while `drain(max)` drains a bounded batch.
- `step()` performs one fill plus one drain and reports whether the lane queued, wrote, blocked, or still has pending data.
- `blocked()` / `pending()` report retained source or sink-side data that must be polled again before the flow is idle.
- Flow payloads must be copied values, stable IDs, or fixed arrays; direct pointers, reference wrappers, `std::span`, `std::string_view`, and standard/result wrappers containing those fail at compile time.

Use this when a data path should be declared as static types and checked at compile time, but task ownership, polling cadence, and hardware policy should stay explicit in application code.

### `arc::Spsc<T, Capacity>`

Bounded lock-free lane for one producer and one consumer.

- `try_push(event)` is producer-only.
- `try_pop(event)` is consumer-only.
- `producer()`, `consumer()`, and `split()` return move-only role endpoints, so task setup can pass only the push or pop side instead of exposing the full lane API.
- `arc::Roles<arc::Spsc<T, Capacity>>` owns the lane privately and exposes only role endpoints, so direct queue mutation is not part of the wrapper's compile-time API.
- `Roles<...>::with_producer(fn)`, `with_consumer(fn)`, and `with_split(fn)` scope endpoint use to one callback and reject callbacks that return endpoints, references, raw pointers, `std::reference_wrapper`, common non-owning views, or standard/result wrappers such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those.
- `RoleEndpoint`, `PushRole<Endpoint, T>`, and `PopRole<Endpoint, T>` let function templates require endpoint-shaped inputs directly.
- `push(span)` and `pop(span)` batch contiguous transfers, wrapping at most once, so burst handoff avoids per-element index publication.
- `size()` and `space()` expose the current ring occupancy and producer room.
- `drain(scratch, fn, max)` batches consumer work without heap allocation; a bool-return callback stops the batch when it returns `false`.
- `cap()` exposes the usable capacity; the backing ring keeps one slot empty.
- `bytes()` exposes the queue object footprint.
- Payloads stay trivially copyable, cannot directly carry borrowed storage such as pointers or views, and the backing ring size remains a power of two.

Use this when event history matters and the ownership contract is exactly one writer and one reader. `arc::Ring<T, Capacity>` is the compatibility alias for the same type.

`arc::Audit<arc::Spsc<T, Capacity>>` keeps the same API, also exposes audited `producer()`, `consumer()`, and `split()` role endpoints, and binds the first producer and consumer it sees so `configASSERT` fires if another task/thread touches that endpoint later.

### `arc::Mpsc<T, Capacity>`

Bounded lock-free fan-in for many producers and one consumer.

- `try_push(event)` can be called by multiple producer tasks.
- `try_pop(event)` is single-consumer and fits Core 0 drain loops.
- `producer()`, `consumer()`, and `split()` return role endpoints; producer handles may be copied across producers, while the consumer handle is move-only.
- `Roles<...>::with_producer(fn)`, `with_consumer(fn)`, and `with_split(fn)` scope endpoint use to one callback and reject callbacks that return endpoints, references, raw pointers, `std::reference_wrapper`, common non-owning views, or standard/result wrappers such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those.
- `drain(scratch, fn, max)` batches consumer work without heap allocation; a bool-return callback stops the batch when it returns `false`.
- `cap()` exposes the power-of-two static capacity.
- `cell_align()`, `cell_bytes()`, and `bytes()` expose the queue's RAM geometry.
- Sequence checks use explicit 32-bit modular deltas, so wrap is handled on the queue clock instead of pointer-width signed math.
- Payloads stay trivially copyable, cannot directly carry borrowed storage such as pointers or views, and capacity remains a power of two.
- Each cell is cache-line isolated to avoid producer/consumer false sharing, so large capacities intentionally spend more internal RAM than a packed queue.
- CAS contention uses a tiny capped `nop` backoff in IRAM instead of immediately hammering the shared head cache line.

Use this when several OS-side tasks with the same scheduling priority need to feed one telemetry or transport owner without a FreeRTOS queue. If producer preemption must never block completed work from another producer, use `arc::Fanin`.

`arc::DenseMpsc<T, Capacity>` keeps the same queue semantics but drops cache-line isolation and packs each cell to the payload's natural alignment. Use it when internal RAM density matters more than worst-case false-sharing avoidance. `arc::Audit<arc::Mpsc<T, Capacity>>` adds audited `producer()`, `consumer()`, and `split()` role endpoints plus a fail-fast single-consumer assertion on top of the cache-line-isolated lane, and `arc::Audit<arc::DenseMpsc<T, Capacity>>` combines both trade-offs. `arc::Roles<arc::Mpsc<T, Capacity>>` owns the lane privately and exposes only producer/consumer endpoints when direct root-lane push/pop should be rejected at compile time.

### `arc::Mux<T, Capacity>`

Compatibility alias for `arc::Mpsc<T, Capacity>`.

Use it when the code should read as a topology: many inputs into one owner. It is declared in `arc/mpsc.hpp`; prefer `arc::Mpsc` when the concurrency contract should be visible at the type site.

### `arc::Fanin<T, Capacity, Producers>`

Static fan-in made from one SPSC lane per producer and one round-robin consumer.

- `try_push<Producer>(event)` is wait-free for that producer lane.
- `push<Producer>(span)` batches producer-side writes into one static lane.
- `producer<Index>()` and `consumer()` return move-only role endpoints for setup code that should pass only one static producer lane or the fan-in drain side.
- `Roles<...>::with_producer<Index>(fn)` and `with_consumer(fn)` scope fan-in endpoint use to one callback and reject callbacks that return endpoints, references, raw pointers, `std::reference_wrapper`, common non-owning views, or standard/result wrappers such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those.
- `try_pop(event)` drains any completed producer without waiting behind another producer's half-finished slot.
- `try_pop(producer, event)` also reports which lane produced the event.
- `pop(span)` batches consumer-side fan-in when producer identity is not needed per item.
- `size<Producer>()` and `space<Producer>()` expose per-lane occupancy and producer room.
- `drain(scratch, fn, max)` accepts either `fn(event)` or `fn(producer, event)`; a bool-return callback stops the batch when it returns `false`.
- `cap()` is the usable per-producer lane capacity, `producers()` is the static lane count, and `bytes()` exposes the full object footprint.
- Payloads stay trivially copyable and cannot directly carry borrowed storage such as pointers, reference wrappers, non-owning views, or standard/result wrappers containing them.

Use this when producer identity is static and tail latency matters more than one global FIFO order.

`arc::Audit<arc::Fanin<T, Capacity, Producers>>` keeps the same API, also exposes audited `producer<Index>()` and `consumer()` role endpoints, and asserts that each lane remains single-producer and the fan-in side stays single-consumer. `arc::Roles<arc::Fanin<T, Capacity, Producers>>` hides root-lane `try_push<Index>`/`try_pop` and exposes only lane producer endpoints plus the fan-in consumer endpoint.

### `arc::RpcLane<Op, RequestPayload, ReplyPayload, RequestCapacity, ReplyCapacity = RequestCapacity>`

Typed request/reply command lane built from static SPSC queues.

- `call(op, payload, serial)` queues a command from the requester side.
- `recv(request)` drains commands on the owner side.
- `reply(serial, status, payload)` sends a structured completion.
- `client()` and `server()` return move-only role endpoints so setup code can pass only requester or owner-side operations.
- `arc::Roles<arc::RpcLane<...>>` owns the lane privately and exposes only `client()` and `server()` endpoints when direct root-lane mutation should be rejected at compile time.
- `Roles<...>::with_client(fn)` and `with_server(fn)` scope RPC endpoint use to one callback and reject callbacks that return endpoints, references, raw pointers, `std::reference_wrapper`, common non-owning views, or standard/result wrappers such as `std::tuple` / `std::pair` / `std::array` / `std::optional` / `std::variant` / `std::expected` / `arc::Result` containing those.
- `RpcClientRole<Endpoint, Op, RequestPayload, Reply>` and `RpcServerRole<Endpoint, Request, Status, ReplyPayload>` make requester and owner endpoint requirements explicit in templates.
- `poll(reply)` drains replies in FIFO order.
- `poll_match(serial, reply)` accepts the requested serial and parks one unmatched reply in a static deferred lane.
- `poll_deferred(reply)` lets the requester recover deferred out-of-order replies.
- Request and reply payloads stay trivially copyable and cannot directly carry borrowed storage such as pointers, reference wrappers, non-owning views, or standard/result wrappers containing them.

Use this for bounded Core 0 to Core 1 control commands when the payloads are trivially copyable and blocking would violate the realtime contract. It is not a dynamic RPC framework: routing, serial allocation, and retry policy stay explicit in application code.

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
- `frequency()`, `duty_permille()`, `rise_ticks()`, and `fall_ticks()` keep the declared compile-time defaults available to board code.

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
- The trailing `AdoptExisting` parameter defaults to `false`; leave it there unless this Arc type is intentionally adopting a bus created elsewhere.
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
- The trailing `AdoptExisting` parameter defaults to `false`; raw ESP-IDF UART drivers must be adopted explicitly.
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

Use this as the low-level USB OTG lane owner before a host/device stack takes over the DWC core. ESP-IDF v6.0.1 in this repo does not ship a TinyUSB component, so Arc exposes the real PHY boundary instead of inventing a partial HID/MSC wrapper.

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

### `arc::vision::PpaSrm`, `PpaFill`, `PpaBlend`, `JpegEncoder`, and `H264Encoder`

Policy-level P4 vision-accelerator planning for frame transforms and compressed output.

- `frame_bytes(width, height, format, stride)` computes bounded raw-frame storage for RGB, YUYV, and YUV420 inputs.
- `PpaSrm`, `PpaFill`, and `PpaBlend` validate source/output frame spans, blocks, scale factors, and formats before calling `policy.srm(...)`, `policy.fill(...)`, or `policy.blend(...)`.
- `JpegEncoder<Width, Height, Quality>` and `H264Encoder<Width, Height, Bitrate, Fps>` keep dimensions, quality, bitrate, and frame rate visible in the type.
- `JpegEncode::submit(policy)` and `H264Encode::submit(policy)` accept caller-owned bitstream buffers and delegate the actual hardware/software driver call to board policy.

Use this between `arc::Dvp` capture, `arc::AxiGraph` movement, and Ethernet/USB streaming when PPA/JPEG/H264 ownership must stay zero-copy. Arc validates buffer geometry and policy contracts; SDK handle lifetime, queue depth, cache maintenance, and component-specific encode settings stay in the board policy.

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
- `lease_coherent(dst, src, bytes)` returns a move-only scoped lease that finishes the coherent transfer on explicit `finish()` or scope exit.
- `lease_coherent(std::move(dst), std::move(src))` moves `arc::CapsBuf` storage into the lease so an async DMA copy cannot outlive those buffers.
- `send_coherent(...)`, `lease_coherent(...)`, and `copy_coherent(...)` accept `arc::CapsBuf<T>` pairs, copying only logical bytes while cache maintenance covers the padded physical storage.
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
- `lines(data, bytes)` returns an `arc::CacheLines` token only for aligned whole-line regions, so strict cache handoff can validate the region once and reuse it.
- `from_raw(arc::unsafe_raw, ...)` and `discard_raw(arc::unsafe_raw, ...)` are not declared by default. Defining `ARC_ENABLE_UNSAFE_CACHE_RAW=1` makes those deprecated escape hatches available for code that explicitly accepts shared-line invalidation risk.
- The unsafe raw escape hatches are unsafe around actively mutated neighbors on the same cache line; use cache-line-aligned buffers or the `_strict` path for live DMA ownership.
- `line(ptr)` returns the cache line size for one address, or zero when the address is not cacheable.
- `arc::CapsBuf<T>` overloads sync the padded physical allocation, while span overloads sync only the exact span byte count.
- `arc::DmaBuf<T, arc::CacheState::cpu>` wraps an `arc::CapsBuf<T>` with CPU-owned cache state; `to_device()` or `discard()` consumes it and returns `arc::DmaBuf<T, arc::CacheState::device>` through `arc::Result`.
- Device-state `arc::DmaBuf` keeps address/size metadata available for hardware handoff but removes CPU `view()`, `data()`, and `operator[]` access until `from_device()` succeeds.
- `take()` returns the raw `arc::CapsBuf<T>` only after ownership is back in the CPU state.

Use this when you are about to hand a buffer to DMA, SPI, I2S, ADC, RMT, or async copy and you want ownership to be explicit instead of implied.

### `arc::prefetch(...)` and `arc::prefetch_write(...)`

Explicit cache preload hints for large linear memory walks.

- `prefetch(ptr)` hints that the CPU will read from the address soon.
- `prefetch_write(ptr)` hints that the CPU will write to the address soon.
- span overloads accept trivially copyable caller-owned buffers.

Use this in long Core 1 loops over PSRAM, DMA frame buffers, or audio blocks when the next cache line can be requested before the current math finishes.

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

`arc::Quadrature<PhaseA, PhaseB, ...>` is the PCNT alias for rotary/linear encoder A/B phase counting. `arc::Encoder<Counter>` adds static `position()` and `sample(dt_us)` helpers that return position, delta, and caller-supplied sample interval without allocating state.

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

### `arc::Etm` and `arc::EtmRoute`

RAII ownership for ESP32-S3 Event Task Matrix event, task, and channel handles.

- `Etm::make(cfg)` allocates one ETM channel.
- `connect(event, task)`, `enable()`, `disable()`, and `disconnect()` expose the native channel lifecycle without hiding ESP-IDF errors.
- `EtmEvent` and `EtmTask` own native event/task handles and delete them on scope exit.
- `EtmRoute<Event, Task>::make(event, task, cfg)` binds movable event/task owners to an enabled channel, keeping the zero-CPU peripheral route alive as one object.

Use this when GPIO, timer, ADC, MCPWM, or other ETM-capable peripherals should trigger each other without an ISR, queue, or Core 1 polling loop.

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
- `read_milli(value)` and `milli_result()` expose milli-Celsius without routing through the panic helper.
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

`SeqReg` is cache-line aligned to avoid false sharing with adjacent state. It publishes into an inactive shadow slot before flipping the even sequence, so readers normally copy from a stable slot, and payload bytes are stored/loaded through relaxed atomics to avoid non-atomic data races if a stale reader overlaps a writer. The sequence counter carries release/acquire visibility without forcing seq-cst ordering. `write()` masks OS-visible interrupts around the odd sequence window on Xtensa targets and uses the same atomic seqlock path without Xtensa masking on RISC-V targets. `read()` inserts a tiny `arc::pause()` between failed snapshots so a fast writer does not turn the losing core into a wasteful full-bus spin.

Use this when `arc::Reg<T>` is too small but a queue would be wasteful.

### `arc::Rcu<T>`

Dual-buffer latest-configuration handoff for large trivially copyable snapshots.

- `staging()` returns the inactive slot for Core 0 or a control owner to fill.
- `publish()` flips the active slot with release/acquire ordering.
- `write(value)` copies into the inactive slot and publishes in one call.
- `read()` returns a const reference to the current active slot without spinning or retrying.

Use this when Core 1 needs to read a large FIR table, routing matrix, or control configuration every loop and retrying a seqlock snapshot would add jitter.

### `arc::LogLane<Capacity>`

Lock-free binary event lane for realtime observability.

- `push(id, payload, aux)` records cycle tick, event ID, and two 32-bit payload words without formatting or UART work.
- `pop(event)` lets Core 0 drain one event.
- `drain(fn, max)` drains a bounded batch into a formatter, socket, file, or test hook; a bool-return callback stops the batch when it returns `false`.
- `dropped()` reports overflow count when the producer outruns the consumer.
- `arc::log_id("name")` creates stable non-zero 32-bit event IDs at compile time.

Use this when Core 1 must report anomalies without calling `ESP_LOGx`, formatting strings, or blocking on UART/Wi-Fi.

### `arc::Text`

Fixed-buffer text writer for Core 0 formatting paths that must stay explicit about memory.

- `append(text)` copies literals, views, or spans into the caller-provided buffer.
- `push(ch)` writes one byte.
- `u32(value)`, `u64(value)`, `usize(value)`, `i32(value)`, and `i64(value)` append decimal numbers without `snprintf`.
- `hex(value, width)` appends lowercase hexadecimal with optional zero padding.
- `json(text)` appends JSON string content with quotes, backslashes, tabs, newlines, carriage returns, and control bytes escaped.
- `span()`, `view()`, and `done()` expose the written prefix without adding a terminator or allocating.
- `format_to(out, "temp={} pc=0x{}", temp, arc::hex(pc, 8))` writes `{}` placeholders, escaped double braces, strings, chars, bools, integers, and explicit hex values into the same caller-owned buffer.

`arc::TraceEventWriter`, `arc::net::HttpServer::text_response(...)`, and `json_response(...)` use this helper internally, so response and trace formatting share one no-heap overflow policy instead of duplicating local append cursors.

### `arc::TraceEventWriter`

Caller-buffer Chrome/Perfetto trace-event JSON formatter for `arc::LogEvent`.

- `json_begin(out)` writes the `{"traceEvents":[` prefix.
- `json_event(out, event, comma, name)` writes one instant event with tick, ID, payload, and aux fields.
- `json_end(out)` closes the JSON document.

Use this on Core 0 after draining `arc::LogLane` or `arc::Postmortem` when you want visual timing analysis in Perfetto without formatting on Core 1.

### `arc::mcap::Writer`

Fixed-buffer MCAP writer for telemetry archives that must not allocate while the control loop is alive.

- `start(profile, library)` writes the leading magic and Header record.
- `schema(value)` and `channel(value)` declare schema bytes, topics, encodings, and metadata from caller-owned spans.
- `message(value)` writes channel, sequence, log time, publish time, and payload bytes.
- `finish(data_crc, footer)` writes DataEnd, Footer, and trailing magic.

Use this after draining binary lanes or ROS2 bridge state into a pre-sized buffer. Arc writes record framing and little-endian fields only; chunking, compression, CRC selection, summary sections, file transport, and ROS2 type support stay in product policy.

### `arc::xrce::Writer`

Fixed-buffer DDS-XRCE message and submessage framing for UDP or serial ROS2 agent bridges.

- `Header{session, stream, seq, key, keyed}` writes the compact XRCE message prefix and optional client key bytes.
- `stream_none`, `stream_best`, and `stream_reliable` name the common XRCE stream lanes.
- `Submsg{id, flags, payload}` wraps one caller-owned XRCE payload behind a submessage header.
- `reserve(id, flags, bytes)` reserves payload space in-place when an XCDR serializer should write directly into the frame.
- `read_header(bytes, keyed)` and `read_sub(bytes, offset)` parse fixed frames back into spans without allocating.

Use this when Arc should own the bounded wire framing but the product still owns XRCE session state, request IDs, object IDs, reliability retries, XCDR payload serialization, and Agent lifecycle.

### `arc::Postmortem<Capacity>`

RTC no-init ring buffer for reboot-surviving diagnostics.

- `boot()` validates or initializes the RTC record and increments the boot counter.
- `append(event)` writes one `arc::LogEvent` into the retained ring.
- `capture(log_lane)` drains a `LogLane` into RTC storage and records its dropped-event count.
- `header()`, `size()`, and `event(i)` expose the retained tail after a soft reboot.

Use this from panic, watchdog, or shutdown hooks when a deterministic system needs a small post-mortem trail before Core 0 formats it later.

### `arc::Probe`, `arc::CycleStats`, `arc::JitterStats`, `arc::DeadlineStats`, and `arc::StallStats`

Cycle-counter instrumentation for hot paths.

- `arc::Probe::now()` snapshots the current Xtensa cycle counter.
- `probe.lap()` returns elapsed cycles since the snapshot.
- `arc::CycleStats::add(cycles)` tracks min/max/total/sample count with no heap.
- `arc::CycleStats::avg()` returns the integer average cycle count.
- `arc::JitterStats::add(delta)` tracks signed early/late cycle deltas.
- `arc::JitterStats::avg_abs()` and `max_abs()` expose absolute jitter without allocating a sample buffer.
- `arc::DeadlineStats::add(elapsed, budget)` tracks signed slack, overrun count, and worst overrun for fixed-period loops.
- `arc::StallStats::add(elapsed, baseline, guard)` tracks excess cycles above a measured baseline plus guard band for shared-bus stall probes.
- `arc::Clock::ns(cycles, mhz)` and `signed_ns(cycles, mhz)` convert cycle measurements to nanoseconds for reports.

Use this when you want to validate the actual cost of a hot path, scheduler period, or realtime budget instead of trusting intuition.

### `arc::TimeSync`

Zero-allocation PI discipliner for peer clock synchronization.

- `TimeSyncSample` carries local send, remote receive, remote send, and local receive timestamps in microseconds.
- `TimeSyncHwSample` carries the same four timestamps in hardware tick domains plus power-of-two tick-to-microsecond shifts.
- `discipline(sample, config)` computes NTP-style offset/delay and applies bounded proportional/integral correction.
- `discipline_hw(sample, config)` converts hardware timestamp samples before applying the same PI discipliner.
- `to_remote(us)` and `to_local(us)` convert timestamps using the filtered offset.

Use this over ESP-NOW or UDP when multiple S3 nodes need a shared microsecond-scale time base without heap state or a background task.

### `arc::dsp`

Hot-loop math helpers for the compute plane.

- `arc::dsp::dot(...)` computes one dot product.
- `arc::dsp::scale(...)` writes `out = in * gain`.
- `arc::dsp::mix(...)` writes `out = lhs + rhs`.
- `arc::dsp::mac(...)` accumulates `acc += in * gain`.
- `arc::dsp::peak(...)` finds the absolute peak.
- `arc::dsp::Fir<T, N>` provides a no-heap FIR state machine with `step(...)` and `run(...)`.
- `arc::dsp::Pid<T>` provides a no-heap PID controller with clamped integral state and derivative-on-measurement behavior to avoid derivative kick.
- `arc::dsp::Biquad<T>` provides a direct-form IIR/FIR section with caller-provided coefficients.
- `arc::dsp::Sos<T, N>` chains fixed second-order sections for deterministic cascaded filtering.
- `arc::dsp::StateSpace<T, States, Inputs, Outputs>` runs fixed-size controller/plant updates with caller-owned state.
- `arc::dsp::inverse(Matrix<T, N, N>)` inverts fixed floating-point matrices without heap state and rejects singular inputs with `ESP_ERR_INVALID_ARG`.
- `arc::dsp::Lqr<T, States, Inputs>` computes finite-horizon LQR gains and control actions over fixed matrix shapes.
- `arc::dsp::Lqr::adapt(...)` identifies one fixed-shape A/B model update from a previous state, input, and observed next state, then recomputes the bounded LQR gain.

Use this when Core 1 is doing signal work and you want aligned buffers plus tight, vector-friendly loops without a heavyweight DSP framework.

### `arc::hls`

Fixed-bound kernel metadata for code that should stay friendly to HLS or RTL
planning flows.

- `KernelSpec` records input/output counts, latency cycles, interface shape, static-bound status, and heapless status.
- `StaticKernel<T>` accepts types that expose `T::hls_spec() -> KernelSpec`.
- `silicon_plan<Kernels...>(target)` produces a constexpr `SiliconPlan` with kernel count, total latency, target, and whether every kernel is static-bound and heapless.
- `StaticLoop<N>::run(fn)` keeps loop trip counts visible to the type system.
- `fir<T, N>(coeffs, samples)` and `dot<T, N>(lhs, rhs)` are fixed-extent examples that reject null spans.

Use this to attach synthesis-oriented metadata to fixed-shape DSP, ML, or
control kernels before a board-specific toolchain decides whether to keep them
on CPU, hand them to HLS, or map them into hardware fabric.

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

Each returns `arc::CapsBuf<T>` with `data()`, `size()`, `bytes()`, and `view()` for the logical payload. Cache/DMA-capable slabs also track their padded physical allocation through `storage_bytes()` and `storage()`, so cache maintenance can cover whole lines without exposing padding as payload.

Standard containers can use the same placement rules through one-word allocators:

- `arc::RamAlloc<T>`
- `arc::PsramAlloc<T>`
- `arc::DmaAlloc<T>`
- `arc::SimdAlloc<T>`

Do not let a capability-allocated `std::vector` reallocate while DMA or a peripheral owns its `.data()` pointer. Reserve capacity before handoff, or prefer `arc::CapsBuf` / `std::array` for active hardware buffers.
Use `arc::OwnedDmaChain<T, N>` when a descriptor ring should own its DMA-capable buffers for the whole ring lifetime instead of binding descriptors to caller-owned spans.

### `arc::FramArena<Bytes>`, `arc::FramRef<T>`, and `arc::FramAlloc<Bytes>`

Policy-backed persistent offset allocation for external FRAM/MRAM state.

- `FramArena<Bytes, Align>` tracks a fixed byte budget and returns aligned `FramSpan` offsets.
- `make<T>()` allocates a typed `FramRef<T>` for trivially copyable state.
- `FramRef<T>::store<Policy>(value)` writes bytes through `Policy::write(offset, bytes)`.
- `FramRef<T>::load<Policy>()` reads bytes through `Policy::read(offset, bytes)`.
- `FramAlloc<Bytes, Align>` is the short alias for the arena when the call site is allocator-shaped.

Use this when a board policy owns an SPI FRAM/MRAM device or a memory-mapped non-volatile window and Arc should only own the fixed layout. Linker sections, memory-mapping registers, power-fail timing, and instruction-pointer resume remain board policy.

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
- `arc::StoreText<N>` owns a fixed, NUL-terminated config string with a `std::string_view` reader.
- `save_text<N>(ns, key, value)` writes text from a `StoreText<N>` or `std::string_view` without heap glue.
- `load_text<N>(ns, key, fallback)` returns fixed text config, uses the fallback only when the key is missing, and rejects oversized persisted data.
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
- `fat_ro(...)`, `fat_info(...)`, `fat_format(...)`, `fat_off()`, and `ro_off(...)` keep FAT control explicit.
- `littlefs(...)`, `littlefs_info(...)`, `littlefs_format(...)`, and `littlefs_off(...)` are available when the managed LittleFS component exposes `esp_littlefs.h`.

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

### `arc::Plane<Work, StackBytes, State, Core>` and `arc::StaticPlane`

Pinned static task for a bound workload.

Your workload defines either:

- `setup()` and `run() noexcept`

or:

- `setup(state)` and `run(state) noexcept`

Use this when you want explicit stateful realtime workers instead of the simpler `Sketch`.

Bound workloads boot as `Plane<...>::boot<&shared>(tag)` so the shared cross-core state is named at compile time and cannot accidentally come from a temporary or stack object. `StaticPlane<Work, StaticRef<&shared, Core>, StackBytes>` is the ergonomic form when the state owner is already declared as a `StaticRef`.

If `Work` or `State` declares `static constexpr std::size_t stack_bytes`, `Plane` uses the larger declaration as its compile-time stack requirement. This makes undersized Core 0/Core 1 task stacks a build error instead of a runtime watchdog or stack canary failure.

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

### `arc::net::Uri`

Heapless URI splitter and percent codec for caller-owned network buffers.

- `parse(text)` returns `arc::Result<arc::net::UriView>` whose spans point into the caller-owned input after checking URI shape, authority userinfo, raw host/literal form, path/query/fragment characters, port range, and percent escapes.
- `endpoint(uri, default_port)` extracts the raw authority host span and a checked 16-bit port for TCP/TLS dial paths, rejecting percent-decoded control bytes or hidden host delimiters.
- `copy_host(out, uri)` writes a safe percent-decoded, NUL-terminated host into caller-owned storage for ESP-IDF APIs that need C strings, including IPv6 zone escapes such as `%25wlan0`.
- `path_query(out, uri)` writes a checked request target, defaulting an empty path to `/` and preserving an explicit empty query.
- `next(query, offset)` iterates query keys and distinguishes missing values from explicit empty values.
- `encode(out, text, space_plus)` and `decode(out, text, plus_space)` convert percent-escaped form/query text without allocating.

Use this before `arc::net::Tcp`, `arc::net::Tls`, or `arc::net::Http` when a URL must be split once while storage, validation, and reconnect policy stay with the application.

### `arc::net::Tcp`

Direct TCP client for Core 0 control and configuration paths.

- `dial(host, port, timeout_ms)` opens one socket and returns `arc::Result<arc::net::Tcp>`.
- `dial(uri, host_scratch, default_port, timeout_ms)` parses a URI endpoint into caller-owned host storage before opening the socket.
- `send(...)` and `recv(...)` return byte counts without hiding partial transfers.
- `recv(..., 0)` restores the POSIX-style blocking receive timeout without changing the send timeout.
- `send_all(...)` loops until the caller-owned buffer is fully written or an error occurs.
- `nonblocking(enable)` switches the native socket between blocking and event-driven use.
- `close()` and `native()` keep socket lifetime explicit.

Use this when a small protocol needs TCP but does not justify a framework-owned task.

### `arc::net::Poll`

Heapless `select()` wrapper for Core 0 socket fan-in.

- `PollItem{sock, want}` describes caller-owned sockets and receives `ready` bits in place.
- `Poll::wait(items, timeout_ms)` returns `arc::Result<std::size_t>` with the number of ready items.
- `poll_forever` requests an infinite wait; `timeout_ms == 0` performs an immediate readiness check.
- `PollInterest::read`, `write`, and `error` can be ORed together without allocating descriptor state.

Use this with `Tcp::nonblocking()` or `Tls::nonblocking()` when one task needs to drive several MQTT, WebSocket, or custom streams without one FreeRTOS stack per connection.

### `arc::net::Pbuf`

Move-only RAII owner for lwIP packet buffers.

- `alloc(layer, bytes, type)` wraps `pbuf_alloc(...)` and returns `arc::Result<arc::net::Pbuf>`.
- `transport_ram(bytes)` is the common transport-layer RAM packet helper.
- `payload()` returns a mutable or const span over the first pbuf payload segment.
- `len()` and `total_len()` expose segment and chain lengths.
- `native()` exposes the raw `pbuf*`; `release()` transfers ownership to lwIP or another owner.
- `reset()` frees the current chain with `pbuf_free(...)`.

Use this when a Core 0 network path needs explicit pbuf lifetime and direct packet-buffer access instead of copying into an intermediate span.

### `arc::net::Tls`

Direct ESP-TLS client transport for Core 0 secure control and telemetry paths.

- `dial(host, port, cfg)` opens a blocking TLS session with caller-provided `esp_tls_cfg_t`.
- `dial(uri, host_scratch, cfg, default_port)` opens the same TLS session from a schemeless authority or `tls`, `https`, `wss`, and `mqtts` URI endpoint.
- `dial_ca(uri, host_scratch, pem, default_port)` keeps the literal CA helper available for URI-based clients.
- `send(...)` and `recv(...)` return byte counts without hiding partial transfers.
- `send_all(...)` loops until the caller-owned buffer is fully written or an error occurs.
- `avail()` exposes decrypted bytes already buffered by the TLS record layer.
- `sockfd()` exposes the underlying lwIP socket descriptor for `arc::net::Poll`.
- `nonblocking(enable)` applies nonblocking mode to that descriptor.
- `close()` and `native()` keep TLS lifetime explicit.

Use this under `arc::net::Mqtt`, `arc::net::Ws`, or a custom stream protocol when you need TLS without adopting a cloud SDK, reconnect loop, or framework-owned task.

### `arc::net::Stream`

Tiny stream composition helpers for transports that expose `send_all(...)` and `recv(...)`.

- `ByteStream<T>` checks the minimal stream surface used by `arc::net::Tcp` and `arc::net::Tls`.
- `AnyStream::bind(io)` erases a concrete byte stream into a small runtime adapter, so protocol code can swap TCP, TLS, test fakes, or board transports without templating every caller.
- `write(...)` forwards a caller-owned buffer to `send_all(...)`.
- `read_exact(...)` loops until a caller-owned buffer is full or the stream closes/errors.
- `write_frame16(...)` and `read_frame16(...)` encode/decode a two-byte big-endian length prefix around a caller-owned payload buffer.
- `frame16(out, payload)` encodes the same record into a caller-owned scratch buffer when the next layer wants a contiguous frame before sending.

Use this when an application protocol needs exact records on top of TCP or TLS but still should not own a reconnect loop, parser task, or heap buffer.

### `arc::pack::Schema<Fields...>`

Fixed binary record packer for telemetry and configuration frames.

- `Schema<...>::bytes` is the compile-time wire size.
- `write<Endian>(out, values...)` writes integer/enum fields into caller-owned output and returns the written span.
- `read<Endian>(in, refs...)` decodes the same schema into caller-owned variables.
- `StructOf<T, &T::field...>` builds the same fixed wire format from explicit data-member pointers.
- `serialize<Endian, Codec>(out, object)` and `deserialize<Endian, Codec>(in, object)` are reflection-ready call sites for struct codecs.
- `Endian::big` is the default; `Endian::little` is available for local protocols.
- multi-byte endian conversion uses `std::byteswap` and `std::memcpy` so unaligned wire fields stay well-defined and map cleanly to compiler byte-swap codegen.
- Field types are limited to non-bool integer and enum scalars up to 64 bits.

Use this when a Core 1/Core 0 or device/network boundary needs a fixed ABI with no heap, no reflection runtime, and compile-time size checks. When compiler static reflection becomes usable, `StructOf` is the explicit adapter that can be generated automatically without changing the caller-buffer wire API.

### `arc::net::Http`

RAII wrapper for ESP-IDF HTTP client sessions.

- `make(config)` returns `arc::Result<arc::net::Http>`.
- `https(config)` and `https(url, base)` require an `https://` URL before constructing the same RAII client.
- `url(...)`, `method(...)`, `header(...)`, and `body(...)` set request fields.
- `perform()` covers the simple one-shot request path.
- `open(...)`, `write(...)`, `fetch()`, `read(...)`, and `close()` expose the streaming path.
- `status()`, `length()`, and `native()` expose response metadata and the raw handle.

Use this for Core 0 REST/config exchanges where HTTP or HTTPS should stay outside the realtime plane. Use `arc::net::Tls` directly when you need a secure raw stream rather than HTTP semantics.

### `arc::net::HttpServer`

Zero-allocation HTTP/1.x request parser and compile-time route matcher for small local config, health, and metrics endpoints.

- `parse(bytes, header_scratch)` returns an `HttpRequestView` whose method, target, headers, and body point into caller-owned receive storage.
- `path(req)` and `query(req)` split the request target into raw path/query views without copying or decoding.
- `find_query(req, "mode")` finds one raw query value and treats bare flags as present empty values.
- `decode_query(scratch, raw)` and `find_query(req, "label", scratch)` decode percent escapes and `+` form spaces into caller-owned scratch storage.
- `find_header(req, "content-length")` performs ASCII case-insensitive header lookup without copying keys.
- `HttpRoute<HttpMethod::get, "/metrics">` creates a compile-time method/path tag with a stable route ID; matching ignores the query string.
- `HttpRouter<Routes...>::dispatch(req, fn)` calls `fn(route_tag)` for the first matching route.
- `text_response(out, 200, "OK", body)` emits a compact close-delimited text response into caller-owned storage.
- `JsonField{"ok", Json::boolean(true)}` declares one fixed JSON object field with string, raw, boolean, signed, unsigned, or null values.
- `json_body(out, fields)` emits only the object body, and `json_response(out, 200, "OK", fields)` emits an `application/json` response with the correct content length.

Use this behind `arc::net::Tcp`, `arc::net::Tls`, or a custom socket accept loop when a device needs a local REST or Prometheus-style surface without importing a task-owning web framework.

### `arc::net::Mqtt`

Thin MQTT 3.1.1 wire codec for caller-owned buffers.

- `connect(...)`, `publish(...)`, `subscribe(...)`, `ping(...)`, and `disconnect(...)` encode packets directly into a caller-provided byte span.
- `parse(...)` splits one MQTT frame out of a receive buffer without hiding the consumed byte count.
- `view(packet)`, `connack(packet)`, and `suback(packet)` decode the common packet bodies without heap allocation.
- `AnyMqtt::arc()` and `AnyMqtt::bind(codec)` expose a small runtime codec adapter so application code can depend on one no-heap MQTT ABI instead of templating every caller on Arc's static codec type.
- `MqttSession` tracks CONNECT/CONNACK state, rolling non-zero packet IDs, keepalive PINGREQ/PINGRESP timing, and broker timeout checks without owning the socket or allocating memory.
- `match(filter, topic)` applies MQTT wildcard rules so subscription routing can stay local and explicit.

Use this when you want MQTT batteries on top of `arc::net::Tcp` or another stream lane without giving the framework ownership of reconnect policy, socket lifetime, or tasking.

### `arc::net::Ws`

Thin WebSocket handshake and frame codec for caller-owned buffers.

- `key(out, nonce)` base64-encodes a caller-owned 16-byte nonce into a `Sec-WebSocket-Key`.
- `accept(out, key)` computes the RFC 6455 `Sec-WebSocket-Accept` response without heap allocation.
- `text(...)`, `binary(...)`, `ping(...)`, `pong(...)`, and `close(...)` encode one frame into a caller-provided byte span.
- `parse(frame, scratch)` decodes one frame, and unmasking stays explicit through the caller-provided scratch span.
- `close_view(frame)` decodes the close code and reason without hiding the wire payload.
- `AnyWs::arc()` and `AnyWs::bind(codec)` expose a small runtime WebSocket codec adapter for callers that should not template on Arc's static codec type.

Use this when you want WebSocket batteries on top of `arc::net::Tcp` or `arc::net::Http` without giving the framework ownership of handshake policy, reconnect loops, or tasking.

### `arc::net::Coap`

Thin CoAP datagram codec for caller-owned buffers.

- `message(...)` encodes one CoAP datagram from explicit type, code, token, options, and payload.
- `ping(...)` and `reset(...)` cover the empty-message path without retyping the header fields.
- `parse(...)` splits token, raw option bytes, and payload out of one received datagram.
- `next(options, offset, number)` walks decoded options in order without heap allocation or hidden iterators.
- `option(...)` and `text(...)` build explicit option descriptors for URI path/query and content-format composition.
- `AnyCoap::arc()` and `AnyCoap::bind(codec)` expose a small runtime CoAP codec adapter for callers that should not template on Arc's static codec type.

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
Use `rotate_lmk(new_key)` while the plane is active to replace the peer LMK through `esp_now_mod_peer()` without tearing down the radio task.

This is the transport to use when you want Wi-Fi silicon and low latency, but not IP, DHCP, or sockets.

### `arc::Wave<Pin, HalfUs, Mhz, Source = arc::ClockSource::systimer>`

Static CPU-owned square-wave generator with explicit timing-source policy.

- The default `ClockSource::systimer` path is wall-clock correct under Dynamic Frequency Scaling.
- `ClockSource::cycle_counter` is rejected at compile time when `CONFIG_PM_ENABLE` is set.
- `ClockSource::locked_cycles` keeps the old cycle-counted hot path for code that holds an `arc::CpuLock` or otherwise guarantees a fixed CPU clock.
- The cycle path keeps its compile-time constants in registers behind an optimizer barrier instead of forcing volatile stack spills.

Use `arc_requires(... time)` when using the default systimer source.

## Outputs

- App binary: `build/arc.bin`
- ELF: `build/arc.elf`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`
- Custom partition CSV: `partitions_16mb.csv`

Arc uses one build directory by default: `build/`.
If you keep a separate release profile in parallel, use `build-release/` deliberately.

## ESP-NOW Example

Arc ships a standalone raw-radio demo at `examples/esp32s3/espnow`.

```bash
cd examples/esp32s3/espnow
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/espnow
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

Arc ships a standalone pulse-counter demo at `examples/esp32s3/count`.

```bash
cd examples/esp32s3/count
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/count
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

Arc ships a standalone RMT loopback capture demo at `examples/esp32s3/trace`.

```bash
cd examples/esp32s3/trace
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/trace
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

Arc also ships a standalone OTA-slot inspection demo at `examples/esp32s3/ota`.

```bash
cd examples/esp32s3/ota
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/ota
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

Arc also ships a standalone hardware-timer demo at `examples/esp32s3/timer`.

```bash
cd examples/esp32s3/timer
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/timer
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

Arc also ships a standalone TWAI/CAN self-test demo at `examples/esp32s3/can`.

```bash
cd examples/esp32s3/can
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/can
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

Arc also ships a standalone DMA memcpy demo at `examples/esp32s3/copy`.

```bash
cd examples/esp32s3/copy
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/copy
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

Arc also ships a standalone LCD_CAM DVP input skeleton at `examples/esp32s3/dvp`.

```bash
cd examples/esp32s3/dvp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/dvp
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

Arc also ships a standalone LCD_CAM Intel 8080 demo at `examples/esp32s3/i80`.

```bash
cd examples/esp32s3/i80
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/i80
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

Arc also ships a standalone cycle-counter demo at `examples/esp32s3/probe`.

```bash
cd examples/esp32s3/probe
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/probe
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

Arc also ships a standalone Sigma-Delta Modulator demo at `examples/esp32s3/sigma`.

```bash
cd examples/esp32s3/sigma
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/sigma
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

Arc also ships a standalone deep-sleep demo at `examples/esp32s3/sleep`.

```bash
cd examples/esp32s3/sleep
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/sleep
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

Arc also ships a standalone internal temperature sensor demo at `examples/esp32s3/temp`.

```bash
cd examples/esp32s3/temp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/temp
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

Arc ships a standalone network demo at `examples/esp32s3/udp`.

```bash
cd examples/esp32s3/udp
. ./env.sh
idf.py menuconfig
idf.py build flash monitor
```

For fish:

```fish
cd examples/esp32s3/udp
source ./env.fish
idf.py menuconfig
idf.py build flash monitor
```

The example is tuned to the same `N16R8` target and uses its own `examples/esp32s3/udp/partitions_16mb.csv`.

The example writes the program directly in `examples/esp32s3/udp/main/app_main.cpp`.

- `Link = arc::Link<Edge, Control, 256>`
- `Core1 = arc::Plane<Pulse, ... , Link>`
- `Core0 = arc::net::Udp<Udp, Link>`

This is the intended style for larger apps: the app composes framework utilities in `main.cpp`, while the heavy transport/runtime machinery stays inside `arc`.

## Space Example

Arc also ships a standalone capacity demo at `examples/esp32s3/space`.

```bash
cd examples/esp32s3/space
. ./env.sh
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/space
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

Arc also ships a standalone hardware-PWM demo at `examples/esp32s3/pwm`.

```bash
cd examples/esp32s3/pwm
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/pwm
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

Arc also ships a standalone MCPWM waveform-plus-capture demo at `examples/esp32s3/pulse`.

```bash
cd examples/esp32s3/pulse
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/pulse
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

Arc also ships a standalone ADC-DMA demo at `examples/esp32s3/scope`.

```bash
cd examples/esp32s3/scope
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/scope
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

Arc also ships a standalone SPI loopback demo at `examples/esp32s3/spi`.

```bash
cd examples/esp32s3/spi
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/spi
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

Arc also ships a standalone I2S duplex demo at `examples/esp32s3/i2s`.

```bash
cd examples/esp32s3/i2s
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/i2s
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

Arc also ships a standalone compute-plane demo at `examples/esp32s3/dsp`.

```bash
cd examples/esp32s3/dsp
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/dsp
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

Arc also ships a standalone complementary-MCPWM demo at `examples/esp32s3/bridge`.

```bash
cd examples/esp32s3/bridge
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/bridge
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

Arc also ships a standalone typed-NVS demo at `examples/esp32s3/store`.

```bash
cd examples/esp32s3/store
. ./env.sh
idf.py build
idf.py size
idf.py size-components
idf.py size-files
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
cd examples/esp32s3/store
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
- `arc::Store::save_text<N>(...)` and `load_text<N>(...)` for fixed-buffer text config
- `arc::Space::flash(...)` and `arc::Space::heap(...)` so persistence and capacity stay visible together

## CI

Arc now ships a build workflow at `.github/workflows/build.yml`.

It builds the root baseline, target-neutral examples, and ESP32-S3 examples, then writes each app binary size into the GitHub Actions step summary so size regressions are visible on every push or PR. Experimental ESP32-S31 examples are discovered but skipped by default because they require `ARC_TARGET=esp32s31`, `ARC_EXPERIMENTAL_ESP32S31=ON`, and preview ESP-IDF target support.

When a change is intentionally documentation-only and the maintainer asks to skip local build/check, CI remains the validation source for the pushed branch.

If `idf.py build` fails in CI, the workflow now dumps the matching `build/log/idf_py_stdout_output_*` and `idf_py_stderr_output_*` files directly into the job log so the real compiler or linker error is visible instead of only the outer `ninja` failure.

The workflow also caches:

- the ESP-IDF checkout under `~/esp-idf`
- the installed Espressif toolchain and Python env under `~/.espressif`
- `~/.ccache` for repeated C/C++ builds

GitHub-hosted runners are still ephemeral, so host packages installed by `apt` do not persist across jobs. Arc now only installs host packages if they are actually missing on the runner.

Before any build runs, CI also executes `./tools/check-repo.sh`. That check fails if generated `sdkconfig` files are tracked, if docs regress to `idf.py set-target ...`, if realtime `loop`/`step` call graphs reach forbidden blocking, queue send/receive/peek, task delay/suspend, timer-pend, task-notify wait, heap, or logging APIs, or if a project stops routing through the shared target selector in `cmake/arc-idf.cmake`.

CI validates clangd coverage with `go run tools/clangd-compile-commands.go --validate-arc-headers -o compile_commands.json`. The Go entrypoint is a thin launcher so local Go installs with partial standard libraries still reach the Python compatibility engine. The Python path normalizes compile commands once, caches compiler probes, auto-selects validation batch size, and falls back to per-header diagnostics only when a batch fails. Use `./tools/clangd-compile-commands.py -o compile_commands.json` for a full local editor refresh, and add `--validate-arc-headers --changed-arc-headers HEAD` for the fast pre-commit clangd gate. Full `--validate-arc-headers` remains available when a broad include/dependency change needs a deliberate all-header pass.

Editor diagnostics use the same database. VS Code and Zed project settings keep C/C++ routed through `clangd`, point it at the repo `compile_commands.json`, and only allow wildcard ESP-IDF query-driver paths so local user toolchain paths do not leak into source control.

CI also executes `./tools/host-tests.sh` before the ESP-IDF build. The host binary now builds through `tests/host/CMakeLists.txt` and Ninja in `build/host`, so local reruns reuse object files instead of recompiling every source from scratch. It exercises SPSC single/batch transfer, MPSC under real producer contention, Fanin round-robin and batch drain behavior, wire codecs, stream helpers, SeqReg snapshots, and DSP/FIR math without flashing hardware.

CI then executes `./tools/host-bench.sh`. The benchmark binary repeats correctness checks inside the timed paths and reports grouped host-runner throughput with explicit `surface` tags for Arc and standard-library rows. It covers Arc SPSC batch/single lanes, MPSC/DenseMPSC, Fanin batch/single drains, SeqReg, stream helpers, DSP, codecs, practical telemetry/control/protocol usage mixes, and standard-library baselines where a fair local baseline exists. The output includes compiler and host context so runner changes are visible next to timing changes. It is not marketed as an ESP32-S3 cycle benchmark or as an Arduino/raw-IDF shootout; it is a regression signal for algorithmic shape, accidental allocation, and gross host-side slowdowns before firmware builds start. `tools/framework-compare.sh` adds host-buildable framework measurements after that step without duplicating the Arc host benchmark in CI, while `examples/esp32s3/bench` is the on-device leg for real ESP32-S3 compare runs.

For hardware numbers, build and flash `examples/esp32s3/bench` on an ESP32-S3. That firmware uses the target cycle counter, groups lanes by area, tags each result with its implementation surface, compares Arc against raw ESP-IDF silicon APIs in the same image, folds internal temp/NVS/OTA plus self-test TWAI loopback into the same run, optionally adds Arduino-ESP32 core paths when the component checkout is available, and still intentionally excludes wired peripheral benchmarks unless a board fixture defines the external devices and pins.

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
- `arc::Lockstep<T>` is where redundant outputs are compared before a safety policy captures or halts a mismatched tick.
- `arc::Scrub<N>` is where a low-priority task can rotate through sealed memory regions; reseal only after legitimate writes, not after a fault.
- `arc::sim::Drive`, `Sense`, `Spi`, `TraceLog`, and `Harness` are the host-side board surface when app logic needs deterministic policy traces, FIFO-fed inputs, or shared-memory-style SPI exchanges without touching ESP32 registers.
- `arc::SeqReg` is the right latest-snapshot lane once a control word no longer fits in `arc::Reg`.
- `arc::dmabuf`, `arc::cachebuf`, `arc::simdbuf`, `arc::ahbbuf`, and `arc::axibuf` are the right way to keep performance-critical heap placement explicit.
- `arc::net::Csi` is the Wi-Fi CSI feature path: capture RF multipath data, extract compact features, then hand the quantized tensor to `arc::ml`.
- `arc::HotPatch` is a low-level board-policy surface: Arc loads and swaps, but the board policy owns the exact detour instruction encoding.
- `arc::ulp::ml` is deliberately tiny: int8 dense inference and semantic wake glue for ULP-side I2C samples, not a full training/runtime stack.
- `arc::crypto::Kyber` is a caller-buffer lattice/KEM surface for on-device ML-KEM plumbing; production key policy and protocol negotiation stay above Arc.
- `arc::crypto::Paillier` composes modular exponentiation/multiplication over `Mpi`-style big integers; key generation and plaintext private-key handling stay off-device.
- `arc::covert` only produces and applies FSK symbols to PWM/Sigma-Delta lanes; protocol framing and operational policy stay with the application.
- `arc::sdr` prepares LCD_CAM pulse streams and board-policy TX hooks; spectrum policy, filtering, and legal transmit decisions stay with the application.
- `arc::swarm::AcousticSlam` provides chirp/TDoA/trilateration primitives, while scheduling, peer trust, and microphone calibration stay with the swarm app.
- `arc::vm::Hypervisor` plans restricted partitions and trap decisions; exact PMS exception vectors remain board-policy code.
- `arc::power::Intermittent` stores a checkpoint image; exact brownout ISR wiring and register restore remain board-policy code.
- `arc::FramRef<T>` stores typed state by offset only; SPI/MRAM timing, linker placement, and restart/resume semantics remain board-policy code.
- `arc::crypto::Puf` extracts and hashes entropy spans; enrollment, helper data policy, and key lifecycle stay above Arc.
- `arc::crypto::Cloak` adds timing and power-noise hooks; threat modeling, lab validation, and key-use policy stay with the product.
- `arc::covert::BlackBox` seals and chains payload metadata; partition selection, PUF enrollment, and flash retention policy stay with the product.
- `arc::Cli` routes fixed command sets; line editing, history, auth, and operator policy stay with the console app.
- `arc::matrix::FlexRoute` chooses spare matrix routes, while board policy still owns pin safety, external wiring, and signal legality.
- `arc::dsp::Wavefront` plans and synthesizes acoustic phases; transducer calibration, drive limits, and ultrasonic safety stay with the app.
- `arc::vision::StarTracker` matches compact triangle signatures; optics calibration, catalog generation, and attitude filtering stay with the app.
- `arc::vision::PpaSrm`, `PpaFill`, `PpaBlend`, `JpegEncoder`, and `H264Encoder` own fixed frame/bitstream validation; P4 SDK handles, interrupts, cache maintenance, and external H264 component choice stay with board policy.
- `arc::net::Fabric` forwards through static TDMA routes; topology provisioning and peer trust stay outside the routing primitive.
- `arc::net::Rdma` validates aligned raw-frame writes; promiscuous-mode filtering, MAC address trust, and DMA descriptor ownership stay with board policy.
- `arc::mmu::DistributedPager` owns deterministic fault-to-cache-line planning; exact LoadStore exception vector patching, peer trust, and MMU remap registers stay with board policy.
- `arc::trace::LiveStream` owns half-full trace chunk handoff; exact TRAX interrupt wiring and USB/W5500 sink policy stay below it.
- `arc::mcap::Writer` owns fixed-buffer MCAP record framing; chunk compression, summary indexes, CRC choice, and ROS2 type binding stay above it.
- `arc::power::Governor` predicts control-loop slack; actual DFS limits, thermal policy, and PM lock names stay with board policy.
- `arc::net::Thread` and `arc::ble::Mesh` validate mesh payload surfaces; radio provisioning, stack lifecycle, and credential storage stay with board policy.
- `arc::SdioSlave` owns coherent queue/finish semantics, and `lease_coherent(...)` ties queue, finish, and RX invalidation to one buffer lifetime; Linux host driver contracts and pin mux stay with board policy.
- `arc::usb::Device` handles Chapter 9 descriptor/status/address/configuration flow and rejects class/vendor setup packets unless the class driver exposes an explicit setup hook; endpoint ISR/DWC2 register policy stays below the class descriptor layer.
- `arc::optical::LiFi` produces Manchester optical symbols; analog comparator thresholds, optics, and eye-safety limits stay with board policy.
- `arc::nav::Eskf`, `arc::ml::Snn`, and `arc::MagLev` are deterministic math/control surfaces, not sensor calibration or plant-identification tools.
- `arc::vm::Jit` owns bounded BPF-to-word translation; real Xtensa opcode selection and executable-memory policy stay behind the micro-assembler hook.
- `arc::vm::HotSwap` sequences verification, translation/loading, protection, and activation hooks; transport, key trust, rollback, and jump policy stay with the product.
- `arc::vm::WasmAot` owns bounded WASM opcode decoding; full module provenance, import policy, and native instruction selection stay behind the AOT policy.
- `arc::x509::Bundle`, `arc::NvsCrypto`, and `arc::secure::SecureBoot` expose security state transitions; CA rollout, key custody, and revocation policy stay with the product.
- `arc::hil::DigitalTwin` wires capture, plant stepping, and encoder output, but the physical model constants and wiring harness remain test-jig policy.
- `arc::chaos` injects bounded faults and records them, but production safety envelopes decide when the monkey is allowed to run.
- `arc::Pipeline` and `arc::PruOut` are where descriptor topology belongs before a board policy connects LCD_CAM/I2S hardware.
- `arc::AxiGraph<Nodes...>` is where hardware graph topology belongs before a board policy binds ETM/AXI triggers.
- `arc::isp` and `arc::vision` are caller-buffer kernels for camera frames; camera register policy still belongs with `arc::I2c` and the board driver.
- `arc::net::DistributedRcu` is the lane for fixed-size fleet state replication; it does not own ESP-NOW retry policy or peer discovery.
- `arc::Crdt<State, Peers>` is the lane for masterless state convergence after radio partitions; it does not own peer trust, clock discipline, or cross-version wire compatibility.
- `arc::Bft<T, Peers>` is the lane for bounded vote certificates; it does not own peer authentication, membership changes, or radio retry policy.
- `arc::net::TsnSchedule<N>` owns time-aware transmit eligibility only; MAC queues, PTP hardware timestamps, and switch configuration stay with board policy.
- `arc::dsp::Beamform` and `arc::dsp::Aec` keep microphone-array math in caller-owned buffers; I2S/PDM capture and playback still stay in the existing I2S lanes.
- `arc::vm::BPF` executes bounded bytecode from fixed spans; use `arc::vm::BpfSandbox` with a board `WorldGuard` policy before running downloaded control blocks.
- `arc::usb_device` describes class-facing descriptor pieces and FIFO bridges; USB class-stack policy stays outside Arc.
- `arc::ulp_cxx` is for tiny low-power policy loops and builder-style wake programs, while full ULP binary load/run ownership stays in `arc::Ulp`.
- `arc::dsp` is intentionally small: it is there to feed the compute plane, not to hide the math under a giant framework.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/esp32s3/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
