# Arc

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

The checked-in defaults are now tuned for `ESP32-S3 N16R8`:

- `16 MB` Quad SPI flash
- `8 MB` Octal PSRAM
- dual OTA app slots sized for a `16 MB` device

- Core 0 is for framework work: boot, storage, drivers, network, logs.
- Core 1 is for the realtime plane: statically allocated, pinned, and kept close to the silicon.
- User programs live in `main/app_main.cpp`.
- `arc::Drive` and `arc::Sense` bind ESP32-S3 dedicated GPIO directly to compile-time types.
- `arc::Burst` streams prebuilt RMT symbols with optional hardware looping.
- `arc::Trace` captures RMT symbols back into SRAM without a CPU sampling loop.
- `arc::Pulse` uses MCPWM for higher-grade waveform generation than LEDC when period and edge placement matter.
- `arc::Bridge` drives complementary MCPWM pairs with explicit dead-time.
- `arc::Capture` timestamps edges in hardware through the MCPWM capture block.
- `arc::Count` offloads pulse accumulation to the PCNT block.
- `arc::Mask` gives an explicit Core-local interrupt barrier when you really need to silence OS-visible interrupts around a tiny hot section.
- `arc::Pwm` binds LEDC hardware PWM directly to compile-time pin/frequency/duty choices.
- `arc::Timer` binds the GPTimer block to a compile-time timebase and optional ISR hook.
- `arc::Tight` runs a masked per-step loop for the rare path that needs tighter jitter than `arc::App`.
- `arc::App` runs a tiny zero-cost program on a chosen core.
- `arc::Link` gives shared event/control state without heap or virtual dispatch.
- `arc::SeqReg` gives multi-word latest-snapshot handoff without queues or torn reads.
- `arc::dmabuf`, `arc::simdbuf`, `arc::ahbbuf`, `arc::axibuf`, and friends make DMA/SIMD/descriptor/RTC-capable heap placement explicit.
- `arc::Space` reports runtime flash, OTA slot, partition, and heap capacity without heap allocation.
- `arc::Ota` wraps staged OTA writes and slot state without raw handle plumbing.
- `arc::Store` gives typed NVS blobs without raw handle plumbing in user code.
- `arc::net::Udp` is a reusable Core 0 transport plane when you opt into `#include "arc/udp.hpp"`.
- `arc::net::EspNow` is a reusable Core 0 raw-radio plane when you opt into `#include "arc/espnow.hpp"`.

## Layout

```text
.
├── CMakeLists.txt
├── cmake
│   └── arc-idf.cmake
├── env.fish
├── env.sh
├── examples
│   ├── bridge
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
│   ├── pwm
│   │   └── README.md
│   ├── timer
│   │   └── README.md
│   ├── espnow
│   │   └── README.md
│   ├── store
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
│               ├── bridge.hpp
│               ├── bus.hpp
│               ├── burst.hpp
│               ├── capture.hpp
│               ├── caps.hpp
│               ├── cfg.hpp
│               ├── clock.hpp
│               ├── count.hpp
│               ├── din.hpp
│               ├── dio.hpp
│               ├── fence.hpp
│               ├── gpio.hpp
│               ├── espnow.hpp
│               ├── plane.hpp
│               ├── ota.hpp
│               ├── pulse.hpp
│               ├── pwm.hpp
│               ├── reg.hpp
│               ├── ring.hpp
│               ├── sketch.hpp
│               ├── space.hpp
│               ├── store.hpp
│               ├── task.hpp
│               ├── timer.hpp
│               ├── trace.hpp
│               ├── udp.hpp
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
- Static FreeRTOS task allocation for the realtime plane
- Core 1 idle watchdog detached so a non-yielding loop can own that core
- Dedicated GPIO output and input on the hot path
- Hardware pulse counting through PCNT
- Hardware symbol streaming through RMT
- Hardware symbol capture through RMT RX
- Hardware MCPWM waveform generation with runtime frequency and duty retuning
- Hardware complementary MCPWM pairs with explicit dead-time
- Hardware MCPWM edge capture for period/high/low measurement
- Hardware PWM offload through LEDC for periodic output that should not burn a core
- Hardware timebase and alarms through GPTimer
- Lock-free telemetry ring and single-word control register
- Slot-aware OTA helper for staged writes and rollback state
- Typed NVS persistence on the Core 0 side

## Programming Model

Arc should feel like a small framework, not like a bag of headers.

- The user-facing program goes in `main/app_main.cpp`.
- `app::boot()` is the normal signature entry you expose for your program.
- `extern "C" void app_main()` stays as the single ESP-IDF boundary.
- For a trivial app, `app::boot()` may be the only thing `app_main()` does.
- For a richer app, `main/app_main.cpp` still shows the program topology directly.

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

`arc::Sketch` remains available as a compatibility alias.

### `arc::Tight<Program, StackBytes, Core = core1, Guard = arc::Critical>`

Masked per-step loop for the extreme path.

Your `Program` type defines:

- `static void setup()`
- `IRAM_ATTR static void step() noexcept`

`Tight` runs `step()` forever and constructs `Guard` around every iteration.

Use this only when the step body is tiny and the jitter budget is tighter than what a normal task iteration should tolerate. `arc::Hard` is a compatibility alias.

### `arc::Link<Event, Control, Capacity>`

Shared state for asymmetric programs.

- `events` is an `arc::Ring<Event, Capacity>`
- `pace` is an `arc::Reg<std::uint32_t>`
- `control` is an `arc::Reg<Control>`

Use it when Core 1 emits ordered events and Core 0 applies latest-wins control.

`arc::Bus` remains available as a compatibility alias.

### `arc::Pwm<Pin, Hz, DutyPermille = 500, Channel = 0, Timer = Channel % 4, Bits = 10>`

Compile-time hardware PWM on ESP32-S3 LEDC.

- `start()` configures timer, channel, pin routing, and starts output.
- `on()` reapplies the default duty.
- `off()` stops output low.
- `pause()` and `resume()` gate the underlying LEDC timer.
- `duty<permille>()` updates duty with a compile-time value.

Use this when the waveform is periodic and the silicon should generate it. Keep `arc::Wave` for cases where the CPU must own every edge.

### `arc::Pulse<Pin, Hz, DutyPermille = 500, Group = 0, ResolutionHz = 10'000'000>`

Compile-time MCPWM waveform wrapper.

- `start()` allocates the timer, operator, comparator, and generator, then starts the hardware waveform.
- `hz(value)` retunes the timer period at runtime.
- `duty(value)` retunes the duty cycle at runtime.
- `on()` and `off()` force a level without tearing down the hardware path.
- `wave()` returns control to the timer/comparator path after a forced level.

Use this when LEDC is too small a hammer and you want a stronger waveform block with explicit period and compare control.

### `arc::Bridge<HighPin, LowPin, Hz, DutyPermille = 500, RiseDelayTicks = 0, FallDelayTicks = 0>`

Compile-time MCPWM complementary pair wrapper.

- `start()` allocates one timer, one operator, one comparator, and two generators.
- `hz(value)` retunes the switching frequency at runtime.
- `duty(value)` retunes the base duty cycle at runtime.
- `wave()` restores complementary switching after a forced state.
- `hi()`, `lo()`, and `off()` force safe states onto the pair without deleting the hardware path.

Use this when the output is no longer “just PWM” and you need a half-bridge style pair with explicit dead-time.

### `arc::Capture<Pin, Hz = 1'000'000, Group = 0, Prescale = 1, Rise = true, Fall = true>`

Compile-time MCPWM capture wrapper.

- `start()` allocates a capture timer and channel and starts timestamping selected edges.
- `ticks()` returns the latest captured timestamp.
- `period()`, `high()`, and `low()` expose the measured edge deltas with no queue allocation.
- `rising()` tells you which edge arrived last.
- `soft()` triggers a software capture for bring-up and inspection.

Use this when you want real edge timestamps without a CPU spin loop or GPIO ISR plumbing.

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

`Handler` supplies:

- `IRAM_ATTR static bool alarm(const gptimer_alarm_event_data_t&) noexcept`

Use this when you want a real hardware timebase or periodic alarm instead of a busy loop.

### `arc::Mask<Level = XCHAL_EXCM_LEVEL>`

Core-local Xtensa interrupt-level guard.

- construct it to raise the interrupt level for the current core
- destroy it to restore the previous level
- `arc::Critical` is the normal “silence OS-visible interrupts” alias
- `arc::Silence` raises all the way to level `15`

Use this only around tiny hot sections where determinism matters more than latency. It is not a substitute for architecture.

### `arc::SeqReg<T>`

Seqlock-style latest-snapshot lane for payloads larger than one word.

- `write(value)` publishes one complete snapshot
- `read()` retries until one stable snapshot is observed
- `try_read(value)` gives you the same read without blocking

Use this when `arc::Reg<T>` is too small but a queue would be wasteful.

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

Typed NVS blob storage for Core 0 work.

- `boot()` initializes NVS and repairs the partition if IDF reports version/free-page mismatch.
- `save(ns, key, value)` writes a trivially copyable value.
- `load(ns, key, value)` reads the exact typed blob back.
- `load_or(ns, key, fallback)` keeps user code short when missing data should fall back cleanly.
- `erase(ns, key)` removes one key and commits.

Use this when you want persistent config without hand-written handle lifetime code.

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

Single-producer single-consumer event lane.

- use for history you cannot collapse
- push/pop are lock-free and power-of-two indexed

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

### `arc::net::Udp<Policy, Bus>`

Reusable Core 0 UDP transport plane.

`Policy` supplies compile-time config:

- `tag`
- `stack`
- `ssid`
- `pass`
- `host`
- `port`

Optional hooks:

- `start(bus)`
- `tick(bus, now)`

This lets you keep network code in the framework while still expressing program-specific control in the app.

Network is intentionally opt-in. Baseline apps only include `arc.hpp`; UDP apps add:

```cpp
#include "arc/udp.hpp"
```

### `arc::net::EspNow<Policy, Bus>`

Reusable Core 0 ESP-NOW plane.

`Policy` supplies compile-time config:

- `tag`
- `stack`
- `channel`
- `peer`

Optional hooks:

- `start(bus)`
- `tick(bus, now)`
- `recv(bus, peer, data, len)`

This is the transport to use when you want Wi-Fi silicon and low latency, but not IP, DHCP, or sockets.

### `arc::Wave<Pin, HalfUs, Mhz>`

Static CPU-owned square-wave generator when you want fixed compile-time timing and explicit spin control on Core 1.

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
- `arc::Space::flash(...)` and `arc::Space::heap(...)` so persistence and capacity stay visible together

## CI

Arc now ships a build workflow at `.github/workflows/build.yml`.

It builds the root baseline and every directory under `examples/*`, then writes each app binary size into the GitHub Actions step summary so size regressions are visible on every push or PR.

The workflow also caches:

- the ESP-IDF checkout under `~/esp-idf`
- the installed Espressif toolchain and Python env under `~/.espressif`
- `~/.ccache` for repeated C/C++ builds

GitHub-hosted runners are still ephemeral, so host packages installed by `apt` do not persist across jobs. Arc now only installs host packages if they are actually missing on the runner.

Before any build runs, CI also executes `./tools/check-repo.sh`. That check fails if generated `sdkconfig` files are tracked, if docs regress to `idf.py set-target ...`, or if a project stops routing through the shared `esp32s3` lock in `cmake/arc-idf.cmake`.

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
- `arc::Wave` is for deliberate CPU-owned edges, not for the common case of “just blink this periodically”.
- `arc::Reg` is better than a queue for latest-wins control words.
- For multi-field control words, prefer explicit fixed-width fields over C++ bitfields.
- `arc::Ring` is better than a register when event history matters.
- `arc::net::EspNow` is the transport to reach for before IP when you want radio latency and do not need routers.
- `arc::Ota` belongs on Core 0 with transports and storage, not in the realtime loop.
- `arc::Burst`, `arc::Trace`, and `arc::Count` are the first place to go when a pin-level job should move from CPU polling into dedicated hardware.
- `arc::Timer` is the right timebase when cycle counters are too core-local or when you need a true peripheral alarm.
- `arc::Mask` is for tiny deterministic sections, not for normal app structure.
- `arc::Tight` is the next step after `arc::App` when you need a masked step loop, not a normal forever-loop task body.
- `arc::SeqReg` is the right latest-snapshot lane once a control word no longer fits in `arc::Reg`.
- `arc::dmabuf`, `arc::cachebuf`, `arc::simdbuf`, `arc::ahbbuf`, and `arc::axibuf` are the right way to keep performance-critical heap placement explicit.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
