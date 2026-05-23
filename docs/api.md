# API Reference

This page mirrors the API section from `README.md` so the same reference is available in the generated website. Start with the module guide when you want the plain-language map, then use this page for the exact public surface.

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
- `native<Policy, HeapPolicy>(plan)` verifies the plan and loads native executable bytes through `arc::HotPatch`.
- `bpf<Policy, JitPolicy>(plan, decoded, out, config)` verifies, decodes BPF bytes, translates them through `arc::vm::Jit`, and asks policy to protect the image.
- `wasm<Policy, AotPolicy>(plan, out, config)` verifies, translates WASM bytes through `arc::vm::WasmAot`, and asks policy to protect the image.
- `activate<Policy>(image)` is a separate explicit policy hook so Arc never jumps into downloaded code by default.

Use this when Core 0 has fetched a signed payload and the product needs one auditable path from verification to PMS/world protection to activation. Signature verification, key trust, executable-memory policy, and rollback remain product policy.

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
- `arc::ulp::Builder<N>` emits a fixed `Program<N>` with steps such as `read_adc(channel)`, `if_greater(threshold)`, `wake_main()`, and `halt()`.
- `Program<N>::run<Policy>()` interprets that bounded program through the same `adc_read(...)` and `wake_main()` policy hooks used by the other ULP C++ helpers.

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

### `arc::Lockstep<T>`

Typed commit point for dual-core redundant control output.

- `match(primary, replica)` performs the pure equality check for trivially copyable output types.
- `commit<Policy>(primary, replica, tick)` returns the agreed output when both cores match.
- On mismatch, `commit` builds `LockstepFault<T>` and calls optional `Policy::capture(fault)` plus optional `Policy::halt(fault)` or `Policy::halt()` before returning `ESP_ERR_INVALID_STATE`.

Use this at the end of paired Core 0/Core 1 control ticks, after both sides computed from identical inputs. Board policy should decide whether the mismatch becomes an IPC halt, postmortem capture, relay shutdown, or another product-specific safety action.

### `arc::sim`

Host-side simulator primitives for app logic tests.

- `sim::Fifo<T, N>` is a fixed-capacity ring for trivially copyable host samples.
- `sim::Drive<Pin, Trace>` mirrors the static `Drive` shape with `out()`, `hi()`, `lo()`, `toggle()`, and `high()`, then calls optional `Trace::drive(pin, level)`.
- `sim::Sense<Pin, N, Trace>` owns a fixed boolean input FIFO. `feed(...)` queues samples, `tick()` consumes one sample, and `high()` / `low()` read the current simulated pin level.
- `sim::TraceLog<N>` records bounded drive/sense/mark events with deterministic tick stamps.
- `sim::Harness<Trace, Inputs...>` resets the trace and input FIFOs, advances each input once per tick, and increments the trace clock.
- `sim::StdoutTrace` prints drive/sense transitions to stdout for host runs; custom trace policies can store deterministic evidence instead.

Use this when a Linux/macOS host build needs to exercise board logic without ESP32 dedicated GPIO registers. Keep the simulator aliases in the host target; firmware targets should continue to use the real `arc::Drive` and `arc::Sense` headers.

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

- `CoreLocal<T, Core::core1>` stores the value privately and only exposes `get<Core::core1>()`, `set<Core::core1>(...)`, and `msg<Core::core1, To>()`.
- Access from the wrong core is absent from the overload set, so misuse fails during template checking instead of becoming a runtime convention.
- `CoreMsg<T, From, To>` carries a copied payload across a queue or mailbox type; only the destination core can call `get<To>()`.
- `accept<Owner>(msg)` applies an addressed message to the destination local slot.

Use this for small ownership-sensitive control or telemetry records where `Spsc`, `SeqReg`, or another lane carries the transfer and the value itself should still remember which core may touch it.

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

Use this when a data path should be declared as static types and checked at compile time, but task ownership, polling cadence, and hardware policy should stay explicit in application code.

### `arc::Spsc<T, Capacity>`

Bounded lock-free lane for one producer and one consumer.

- `try_push(event)` is producer-only.
- `try_pop(event)` is consumer-only.
- `producer()`, `consumer()`, and `split()` return move-only role endpoints, so task setup can pass only the push or pop side instead of exposing the full lane API.
- `arc::Roles<arc::Spsc<T, Capacity>>` owns the lane privately and exposes only role endpoints, so direct queue mutation is not part of the wrapper's compile-time API.
- `push(span)` and `pop(span)` batch contiguous transfers, wrapping at most once, so burst handoff avoids per-element index publication.
- `size()` and `space()` expose the current ring occupancy and producer room.
- `drain(scratch, fn, max)` batches consumer work without heap allocation; a bool-return callback stops the batch when it returns `false`.
- `cap()` exposes the usable capacity; the backing ring keeps one slot empty.
- `bytes()` exposes the queue object footprint.
- Payloads stay trivially copyable and the backing ring size remains a power of two.

Use this when event history matters and the ownership contract is exactly one writer and one reader. `arc::Ring<T, Capacity>` is the compatibility alias for the same type.

`arc::Audit<arc::Spsc<T, Capacity>>` keeps the same API, also exposes audited `producer()`, `consumer()`, and `split()` role endpoints, and binds the first producer and consumer it sees so `configASSERT` fires if another task/thread touches that endpoint later.

### `arc::Mpsc<T, Capacity>`

Bounded lock-free fan-in for many producers and one consumer.

- `try_push(event)` can be called by multiple producer tasks.
- `try_pop(event)` is single-consumer and fits Core 0 drain loops.
- `producer()`, `consumer()`, and `split()` return role endpoints; producer handles may be copied across producers, while the consumer handle is move-only.
- `drain(scratch, fn, max)` batches consumer work without heap allocation; a bool-return callback stops the batch when it returns `false`.
- `cap()` exposes the power-of-two static capacity.
- `cell_align()`, `cell_bytes()`, and `bytes()` expose the queue's RAM geometry.
- Sequence checks use explicit 32-bit modular deltas, so wrap is handled on the queue clock instead of pointer-width signed math.
- Payloads stay trivially copyable and capacity remains a power of two.
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
- `try_pop(event)` drains any completed producer without waiting behind another producer's half-finished slot.
- `try_pop(producer, event)` also reports which lane produced the event.
- `pop(span)` batches consumer-side fan-in when producer identity is not needed per item.
- `size<Producer>()` and `space<Producer>()` expose per-lane occupancy and producer room.
- `drain(scratch, fn, max)` accepts either `fn(event)` or `fn(producer, event)`; a bool-return callback stops the batch when it returns `false`.
- `cap()` is the usable per-producer lane capacity, `producers()` is the static lane count, and `bytes()` exposes the full object footprint.

Use this when producer identity is static and tail latency matters more than one global FIFO order.

`arc::Audit<arc::Fanin<T, Capacity, Producers>>` keeps the same API, also exposes audited `producer<Index>()` and `consumer()` role endpoints, and asserts that each lane remains single-producer and the fan-in side stays single-consumer. `arc::Roles<arc::Fanin<T, Capacity, Producers>>` hides root-lane `try_push<Index>`/`try_pop` and exposes only lane producer endpoints plus the fan-in consumer endpoint.

### `arc::RpcLane<Op, RequestPayload, ReplyPayload, RequestCapacity, ReplyCapacity = RequestCapacity>`

Typed request/reply command lane built from static SPSC queues.

- `call(op, payload, serial)` queues a command from the requester side.
- `recv(request)` drains commands on the owner side.
- `reply(serial, status, payload)` sends a structured completion.
- `client()` and `server()` return move-only role endpoints so setup code can pass only requester or owner-side operations.
- `arc::Roles<arc::RpcLane<...>>` owns the lane privately and exposes only `client()` and `server()` endpoints when direct root-lane mutation should be rejected at compile time.
- `poll(reply)` drains replies in FIFO order.
- `poll_match(serial, reply)` accepts the requested serial and parks one unmatched reply in a static deferred lane.
- `poll_deferred(reply)` lets the requester recover deferred out-of-order replies.

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

### `arc::AxiGraph<Nodes...>`

Compile-time hardware graph planner for descriptor-to-descriptor peripheral chains.

- Each node exposes `input()` and `output()` returning `arc::AxiPort`.
- `plan(nodes...)` validates adjacent output/input DMA ports and returns fixed `AxiEdge` records.
- `link(nodes...)` connects each upstream DMA tail to the next node's DMA head.
- `boot(policy, nodes...)` links descriptors, calls optional `policy.link(edge_index, edge)` for board ETM/AXI trigger setup, then optional `policy.arm(node_index, node)` for each node.

Use this when board policy wants a camera-to-transform-to-sink path expressed once while keeping the actual register trigger setup outside Arc. `AxiGraph` proves topology and descriptor continuity; runtime throughput still needs hardware capture on the target board.

### `arc::Scrub<Regions>`

Bounded background memory scrubber for fixed IRAM, DRAM, PSRAM, or code/data windows.

- `watch(index, bytes, tag)` records a span pointer, byte length, tag, and CRC-32 seal.
- `scan<Policy>()` checks one registered region from the rotating cursor and returns a `ScrubSample`.
- `step<Policy>(budget)` runs a bounded number of scans for a low-priority static task.
- On mismatch, `scan` stores `last`, calls optional `Policy::capture(fault)` plus optional `Policy::halt(fault)` or `Policy::halt()`, and returns `ESP_ERR_INVALID_STATE`.
- `refresh(index)` intentionally reseals a region after legitimate writes.

Use this for safety cases that need silent-memory-corruption detection without heap state. Register only stable regions, run `step()` from non-realtime background work, and treat a mismatch as product policy rather than automatically mutating the seal.

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

### `arc::Plane<Work, StackBytes, State, Core>`

Pinned static task for a bound workload.

Your workload defines either:

- `setup()` and `run() noexcept`

or:

- `setup(state)` and `run(state) noexcept`

Use this when you want explicit stateful realtime workers instead of the simpler `Sketch`.

Bound workloads boot as `Plane<...>::boot<&shared>(tag)` so the shared cross-core state is named at compile time and cannot accidentally come from a temporary or stack object.

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

### `arc::Crdt<State, Peers>`

Heapless state-based CRDT envelope for UDP, ESP-NOW, or custom fixed-frame swarm state.

- `arc::GCounter<T, Peers>` stores one monotonic unsigned cell per peer, rejects per-cell overflow in `add(...)`, and merges by per-peer maximum.
- `arc::PnCounter<T, Peers>` composes increment/decrement G-counters and returns a signed converged value without allocating.
- `arc::LwwReg<T, Clock>` stores one trivially copyable value with a logical timestamp and deterministic node-ID tie break.
- `arc::Crdt<State, Peers>` wraps any trivially copyable state with a `merge(...)` method, emits a fixed `Frame`, parses the same ABI back from caller-owned bytes, and tracks a bounded peer table.

Use this for small fleet state that must converge after radio partitions without a master node. The raw frame ABI is meant for matching Arc firmware builds; product protocols that cross compiler or version boundaries should wrap it in `arc::pack`.

### `arc::Bft<T, Peers>`

Bounded Byzantine-fault-tolerant quorum collection for critical fleet decisions.

- `Bft::faults()` derives the tolerated Byzantine fault count from the fixed peer count, and `Bft::quorum()` returns `2f + 1`.
- `vote(node, value, digest)` emits a fixed vote frame for the current view/sequence.
- `ingest(vote)` accepts one vote per node, treats duplicate identical votes as idempotent, and rejects equivocation from the same node.
- `cert()` returns a quorum certificate once enough peers agree on the same value and digest.
- `bytes(vote)` and `parse(bytes)` expose the raw fixed ABI for UDP, ESP-NOW, or a caller-owned authenticated envelope.

Use this for small abort/commit/role-transfer decisions where the application already owns peer identity, digest policy, and authentication. It is a bounded vote collector, not a radio task or membership service.

### `arc::net::TsnSchedule<Entries>`

Time-aware Ethernet transmit gate checks for deterministic MAC policies.

- `set(index, gate)` installs a fixed `TsnGate` with cycle offset, duration, guard time, and traffic class.
- `window(now_ns, traffic)` returns the active `TsnWindow` for that class when the current nanosecond timestamp is inside a guarded gate.
- `allow(now_ns, traffic)` is the boolean fast path for MAC queues.
- `next_open(now_ns, traffic)` returns the next eligible nanosecond timestamp for that traffic class.

Use this after a PTP-disciplined clock has provided a shared time base. Arc evaluates the schedule without heap state; the board policy still owns MAC queues, timestamp capture, switch configuration, and whether the schedule maps to 802.1Qbv or a simpler private gate table.

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
