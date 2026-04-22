# Arc

Arc is an ESP-IDF base for ESP32-S3 firmware that treats Core 0 and Core 1 differently on purpose.

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
- `arc::Result<T>` is an opt-in `std::expected<T, esp_err_t>` alias for runtime operations that should not hard-panic.
- `arc::Cache` makes DMA/PSRAM cache coherency explicit at the call site, while `arc::Copy::send_coherent(...)` and `finish_coherent(...)` cover the common async-memcpy handoff without blocking useful CPU work.
- `arc::Can` binds the ESP32-S3 TWAI/CAN controller with ISR-backed RX handoff.
- `arc::Burst` streams prebuilt RMT symbols with optional hardware looping.
- `arc::Trace` captures RMT symbols back into SRAM without a CPU sampling loop.
- `arc::Pulse` uses MCPWM for higher-grade waveform generation than LEDC when period and edge placement matter.
- `arc::Bridge` drives complementary MCPWM pairs with explicit dead-time.
- `arc::Capture` timestamps edges in hardware through the MCPWM capture block.
- `arc::Scope` streams ADC data through the digital controller and DMA path.
- `arc::Copy` offloads memory movement to the async DMA memcpy engine.
- `arc::Dvp` captures parallel camera frames through the ESP32-S3 LCD_CAM DMA path.
- `arc::I80Bus` and `arc::I80` drive the ESP32-S3 LCD_CAM Intel 8080 DMA path with exact transfer tickets.
- `arc::I2cBus` and `arc::I2c` bind ESP32-S3 I2C master buses and devices with recoverable init paths.
- `arc::SpiBus` and `arc::Spi` drive DMA-capable SPI transfers with ticketed queue/finish ownership.
- `arc::I2s` owns standard-mode I2S channels and DMA event counters with both raw `esp_err_t` and opt-in `arc::Result` APIs.
- `arc::Uart` binds ESP32-S3 UART ports, pins, framing, and buffers with fixed storage and typed read/write APIs.
- `arc::Usb` binds the ESP32-S3 USB Serial/JTAG lane with typed byte IO.
- `arc::Sd` mounts ESP32-S3 SD/MMC cards through FAT and keeps raw sector access explicit.
- `arc::Fs` mounts SPIFFS and FAT-on-flash VFS paths with fixed handle ownership.
- `arc::File` gives RAII VFS/POSIX file I/O without leaking `FILE*` ownership into app code.
- `arc::Count` offloads pulse accumulation to the PCNT block.
- `arc::dsp` adds hot-loop math kernels that pair naturally with `arc::simdbuf` and Core 1.
- `arc::Probe` reads the Xtensa cycle counter so hot paths can be measured, not guessed.
- `arc::Mask` gives an explicit Core-local interrupt barrier when you really need to silence OS-visible interrupts around a tiny hot section.
- `arc::Pwm` binds LEDC hardware PWM directly to compile-time pin/frequency/duty choices.
- `arc::Sigma` binds the ESP32-S3 Sigma-Delta Modulator to a compile-time GPIO and sample rate.
- `arc::Timer` binds the GPTimer block to a compile-time timebase and optional ISR hook.
- `arc::Sleep` wraps wake causes, timer wake, power-domain policy, light sleep, and deep sleep.
- `arc::Temp` reads the ESP32-S3 internal temperature sensor for thermal telemetry.
- `arc::Tight` runs a masked per-step loop with optional cycle-budget overrun telemetry for the rare path that needs tighter jitter than `arc::App`.
- `arc::App` runs a tiny zero-cost program on a chosen core.
- `arc::Link` gives shared event/control state without heap or virtual dispatch.
- `arc::Spsc` gives a bounded lock-free lane for one producer and one consumer; `arc::Ring` remains the terse compatibility alias.
- `arc::Mpsc` gives bounded lock-free fan-in when several producers must feed one Core 0 consumer; `arc::Mux` is the terse topology alias.
- `arc::Fanin` gives per-producer SPSC lanes when producer preemption must not block completed work from other producers.
- `arc::SeqReg` gives multi-word latest-snapshot handoff without queues or torn reads.
- `arc::dmabuf`, `arc::simdbuf`, `arc::ahbbuf`, `arc::axibuf`, and one-word STL allocators make DMA/SIMD/descriptor/RTC-capable heap placement explicit.
- `arc::Space` reports runtime flash, OTA slot, partition, and heap capacity without heap allocation.
- `arc::Ota` wraps staged OTA writes and slot state without raw handle plumbing.
- `arc::Store` gives typed NVS blobs and fixed-buffer strings without raw handle plumbing in user code.
- `arc::net::Radio` is the shared Core 0 Wi-Fi owner, so UDP and ESP-NOW do not each re-create NVS, netif, event loop, and Wi-Fi driver state.
- `arc::net::Tcp` gives direct TCP socket clients for Core 0 control/config paths.
- `arc::net::Http` gives RAII ownership for ESP-IDF HTTP client sessions.
- `arc::net::Udp` is a reusable Core 0 transport plane when you opt into `#include "arc/udp.hpp"`.
- `arc::net::EspNow` is a reusable Core 0 raw-radio plane when you opt into `#include "arc/espnow.hpp"`.

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
│               ├── fanin.hpp
│               ├── fence.hpp
│               ├── file.hpp
│               ├── fs.hpp
│               ├── gpio.hpp
│               ├── http.hpp
│               ├── espnow.hpp
│               ├── i2c.hpp
│               ├── i80.hpp
│               ├── i2s.hpp
│               ├── mpsc.hpp
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
│               ├── sleep.hpp
│               ├── sketch.hpp
│               ├── sigma.hpp
│               ├── spi.hpp
│               ├── space.hpp
│               ├── spsc.hpp
│               ├── store.hpp
│               ├── task.hpp
│               ├── temp.hpp
│               ├── tcp.hpp
│               ├── timer.hpp
│               ├── topology.hpp
│               ├── trace.hpp
│               ├── uart.hpp
│               ├── udp.hpp
│               ├── usb.hpp
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
- Hardware complementary MCPWM pairs with explicit dead-time
- Hardware MCPWM edge capture for period/high/low measurement
- Hardware ADC streaming through the digital controller and DMA
- Hardware async memory copy through the ESP32-S3 DMA memcpy path
- Hardware LCD_CAM DVP camera input through DMA-backed frame buffers
- Hardware LCD_CAM Intel 8080 parallel output through DMA-backed panel IO
- Hardware I2C master bus/device binding for sensors, EEPROMs, displays, and control chips
- Hardware SPI transfers with explicit bus/device composition and DMA-capable paths
- Hardware I2S streaming with duplex DMA and event counters
- Hardware UART serial lanes for GPS, modems, consoles, and legacy links
- Hardware USB Serial/JTAG byte IO for cabled control channels
- Hardware SD/MMC FAT mounting and raw sector access
- Hardware PWM offload through LEDC for periodic output that should not burn a core
- Hardware Sigma-Delta pulse-density output with IRAM-safe density updates enabled
- Hardware timebase and alarms through GPTimer
- Deep-sleep and light-sleep entry with explicit wake-source and power-domain policy
- Hardware die-temperature telemetry for thermal guard logic
- Lock-free SPSC/MPSC queues and single-word control register
- Slot-aware OTA helper for staged writes and rollback state
- Typed NVS persistence and bounded string loading on the Core 0 side
- SPIFFS and FAT-on-flash VFS mounting
- RAII file handles for mounted VFS/FAT/SPIFFS/LittleFS paths
- Direct TCP clients for small control/config protocols
- RAII HTTP client sessions for REST/config exchanges
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
- `copy`
- `mcpwm`
- `rmt`
- `pcnt`
- `ledc`
- `gptimer`
- `http`
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
- `space`
- `fs`
- `tcp`
- `udp`
- `uart`
- `usb`
- `espnow`

Add `gpio` when you use `arc::Drive`, `arc::Sense`, or raw `arc::Gpio`, because those pin APIs depend on the GPIO driver headers.

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

Peripheral wrappers also claim physical silicon at `init()` time. Two different `arc::Uart`, `arc::I2cBus`, `arc::I2c`, `arc::SpiBus`, or CS-backed `arc::Spi` template aliases cannot silently own the same hardware with different type parameters; the later claim returns `ESP_ERR_INVALID_STATE`.

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
- Exposes feature booleans such as `wifi`, `ble`, `simd`, `async_memcpy`, `ahb_gdma`, `dedicated_gpio`, `lcd_i80`, `lcdcam_dvp`, and `ulp_riscv`.
- Exposes hardware counts such as `gpio_pins`, `adc_units`, `ledc_channels`, `spi_peripherals`, `rmt_words`, `uart_ports`, and `sdmmc_slots`.
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
- `cap()` exposes the power-of-two static capacity.
- Payloads stay trivially copyable and capacity remains a power of two.

Use this when event history matters and the ownership contract is exactly one writer and one reader. `arc::Ring<T, Capacity>` is the compatibility alias for the same type.

### `arc::Mpsc<T, Capacity>`

Bounded lock-free fan-in for many producers and one consumer.

- `try_push(event)` can be called by multiple producer tasks.
- `try_pop(event)` is single-consumer and fits Core 0 drain loops.
- `drain(scratch, fn, max)` batches consumer work without heap allocation.
- `cap()` exposes the power-of-two static capacity.
- Sequence checks use explicit 32-bit modular deltas, so wrap is handled on the queue clock instead of pointer-width signed math.
- Payloads stay trivially copyable and capacity remains a power of two.

Use this when several OS-side tasks with the same scheduling priority need to feed one telemetry or transport owner without a FreeRTOS queue. If producer preemption must never block completed work from another producer, use `arc::Fanin`.

### `arc::Mux<T, Capacity>`

Compatibility alias for `arc::Mpsc<T, Capacity>`.

Use it when the code should read as a topology: many inputs into one owner. It is declared in `arc/mpsc.hpp`; prefer `arc::Mpsc` when the concurrency contract should be visible at the type site.

### `arc::Fanin<T, Capacity, Producers>`

Static fan-in made from one SPSC lane per producer and one round-robin consumer.

- `try_push<Producer>(event)` is wait-free for that producer lane.
- `try_pop(event)` drains any completed producer without waiting behind another producer's half-finished slot.
- `try_pop(producer, event)` also reports which lane produced the event.
- `drain(scratch, fn, max)` accepts either `fn(event)` or `fn(producer, event)`.
- `cap()` is the per-producer lane capacity and `producers()` is the static lane count.

Use this when producer identity is static and tail latency matters more than one global FIFO order.

### `arc::Pwm<Pin, Hz, DutyPermille = 500, Channel = 0, Timer = Channel % 4, Bits = 10>`

Compile-time hardware PWM on ESP32-S3 LEDC.

- `start()` configures timer, channel, pin routing, and starts output.
- `on()` reapplies the default duty.
- `off()` stops output low.
- `pause()` and `resume()` gate the underlying LEDC timer.
- `duty<permille>()` updates duty with a compile-time value.

Use this when the waveform is periodic and the silicon should generate it. Keep `arc::Wave` for cases where the CPU must own every edge.

### `arc::Sigma<Pin, SampleHz = 1'000'000, Initial = 0>`

Compile-time Sigma-Delta Modulator output.

- `start()` creates and enables one SDM channel routed to the pin.
- `write(value)` updates pulse density in the hardware driver.
- `write<Value>()` range-checks the density at compile time.
- `zero()`, `high()`, and `low()` are shorthand targets.
- `fast(value)` is the thinnest post-boot path and returns `esp_err_t` instead of panicking.

Use this when you want hardware pulse-density output or a low-pass-filtered analog-ish voltage without burning CPU cycles on a PWM loop.

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

### `arc::Adc<Io, Atten = ADC_ATTEN_DB_12, Width = SOC_ADC_DIGI_MAX_BITWIDTH>`

Compile-time ADC pad descriptor for `arc::Scope`.

- `io()` returns the GPIO number.
- `atten()` returns the attenuation used for the digital pattern.
- `width()` returns the configured ADC bit width.

Use this as a pad specifier, not as a runtime object.

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
- `host()`, `sclk()`, `mosi()`, and `miso()` keep the routing obvious in user code.
- host claims reject incompatible second aliases for the same physical SPI block.
- The SPI ISR is pinned to CPU0 by default so transport work naturally stays off the realtime core.

Use this when multiple devices should share one bus or when bus routing should stay explicit and reusable.

### `arc::Spi<Bus, Cs = -1, Hz = SPI_MASTER_FREQ_20M, Mode = 0, Queue = 4, ...>`

Compile-time SPI device wrapper.

- `init()` adds one device and returns `esp_err_t`.
- `boot()` adds one device on top of a compile-time `arc::SpiBus`.
- `off()` removes the device and releases its CS claim.
- `send(...)`, `recv(...)`, `xfer(...)`, and `poll(...)` cover the common synchronous paths.
- `queue(...)` and `wait(...)` expose the interrupt-driven transaction path when you want multiple jobs in flight.
- `Move` and `StrictMove` are ticket objects that own queued transaction storage until `finish(...)`.
- `queue_coherent(move, tx, rx, bytes)` flushes TX, safely discards whole RX cache lines, and queues one DMA transfer.
- `finish_coherent(move)` waits for that exact SPI transfer and invalidates RX before the CPU reads it.
- `acquire()` and `release()` give explicit bus ownership when CS must stay held.

Use this when bytes should move through the SPI engine and DMA path instead of a software bit loop.

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

### `arc::Can<Tx, Rx, Bitrate = 500'000, Queue = 8, RxDepth = 8, ...>`

Compile-time ESP32-S3 TWAI/CAN node wrapper.

- `boot()` creates one on-chip TWAI node and binds ISR callbacks.
- `start()` and `stop()` enable or disable bus participation.
- `frame(id, payload, ext, remote)` builds an owning classic CAN frame.
- `send(frame, timeout_ms)` queues a frame; keep the frame alive until TX completes.
- `send_wait(frame, timeout_ms)` queues and waits for completion before returning.
- `recv(frame)` drains the ISR-backed lock-free `arc::Spsc` RX lane.
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

### `arc::Uart<Port, Tx, Rx, Rts = UART_PIN_NO_CHANGE, Cts = UART_PIN_NO_CHANGE, ...>`

Compile-time UART wrapper.

- `init()` configures the port, pins, framing, buffers, and driver, then returns `esp_err_t`.
- `boot()` keeps the fail-fast path for required serial links.
- `off()` deletes the driver and releases the port claim.
- `write(...)` and `read(...)` expose `arc::Result<std::size_t>` ergonomic overloads.
- `available(...)`, `wait(...)`, `flush()`, and `baud(...)` expose the common runtime controls.

Use this for GPS receivers, modem AT links, binary serial protocols, and board-level debug channels that should not leak raw UART driver calls into app code.

### `arc::Usb<Tx = 256, Rx = 256>`

Compile-time USB Serial/JTAG wrapper.

- `init()` installs the driver with fixed TX/RX ring sizes and returns `esp_err_t`.
- `boot()` keeps the fail-fast path for required cabled control.
- `write(...)` and `read(...)` expose `arc::Result<std::size_t>` byte-count overloads.
- `connected()`, `installed()`, `wait(...)`, and `off()` expose runtime state and teardown.

Use this for ESP32-S3 USB console/control traffic when the native USB Serial/JTAG peripheral is the lane.

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
- `boot()` creates one DVP controller with compile-time sync/data routing.
- `start()`, `stop()`, `pause()`, and `resume()` gate the capture lane.
- `buffer<T>(count)` allocates a DMA-capable camera frame buffer.
- `grab(frame, &bytes, timeout_ms)` receives one frame into user-owned storage.
- `convert(in, out, ...)` exposes the hardware format-conversion hook.
- `frames()` and `bytes()` expose received-frame counters.

Use this for camera or parallel input streams that should land in RAM through LCD_CAM/DMA instead of CPU sampling.

### `arc::I80Bus<arc::Lines<...>, Dc, Wr>` and `arc::I80<Bus, Cs>`

Compile-time Intel 8080 parallel output using the ESP32-S3 LCD_CAM block.

- `arc::Lines<...>` declares 8 or 16 data GPIOs.
- `I80Bus::boot()` creates one DMA-backed I80 bus.
- `I80::boot()` creates one panel IO endpoint.
- `param(cmd, data, bytes)` sends command parameters.
- `color(cmd, data, bytes)` queues a DMA-backed payload transfer.
- `Ticket` captures the exact queued color transfer so `finish(ticket)` does not wait on unrelated later transfers.
- `color_coherent(ticket, cmd, buffer)` flushes the draw buffer before LCD_CAM owns it.
- `buffer<T>(count)` allocates a draw buffer with the alignment/caps expected by the I80 path.
- `sent()`, `done()`, `bytes()`, `idle()`, `ready(ticket)`, `wait()`, and `finish(ticket)` expose the DMA queue state.

Use this for display or parallel-device throughput that should be owned by LCD_CAM/DMA, not by a GPIO loop.

### `arc::Copy<Backlog = 4, BurstBytes = 64, Weight = 0, Backend = arc::CopyBackend::auto_dma>`

Compile-time async DMA memcpy wrapper.

- `boot()` installs one async memcpy driver instance.
- `send(dst, src, bytes)` queues a non-blocking DMA copy and returns immediately after submission.
- `copy(dst, src, bytes)` submits the transfer and spins until the completion counter reaches the target.
- `send_coherent(ticket, dst, src, bytes)` flushes the source cache, discards destination cache lines, queues the DMA copy, and stores the exact completion target in `ticket`.
- `finish_coherent(ticket)` waits for that exact transfer and invalidates the destination cache before CPU reads it.
- `ready(ticket)` reports whether that exact transfer has completed.
- `copy_coherent(dst, src, bytes)` is the blocking one-call form built on the non-blocking ticket path.
- coherent copy destination buffers must cover whole cache lines; the `_strict` variants also require the source side to be cache-line aligned.
- `sent()`, `done()`, `bytes()`, and `idle()` expose lock-free counters without FreeRTOS queues.
- `arc::CopyBackend::ahb` pins the backend to AHB-GDMA on ESP32-S3.

Use this when bytes should move through the DMA memcpy engine instead of burning CPU cycles on a software copy loop.

### `arc::Cache`

Explicit cache coherency helpers for DMA and external-memory paths.

- `to_device(data, bytes)` writes dirty cache lines back before hardware reads a buffer.
- `from_device(data, bytes)` invalidates only whole cache-line-aligned buffers after hardware writes a buffer.
- `discard(data, bytes)` writes back and invalidates only whole cache-line-aligned buffers when ownership moves away from the CPU.
- `from_device_unaligned(...)` and `discard_unaligned(...)` are the explicit escape hatches when the caller accepts shared-line invalidation risk.
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

### `arc::Sleep`

Power-state helper for the Core 0 side.

- `after_us(value)` and `after<Value>()` arm timer wake.
- `causes()` returns the wake-source bitmask from the previous sleep.
- `woke(source)` tests one wake source.
- `keep(domain)`, `auto_power(domain)`, and `power_down(domain)` set sleep power-domain policy.
- `light()` enters light sleep and returns after wake.
- `deep()` enters deep sleep and does not return.

Use this when the most efficient loop is no loop at all.

### `arc::Temp<Min = -10, Max = 80>`

Internal ESP32-S3 die-temperature helper.

- `start()` installs and enables the temperature sensor once.
- `stop()` disables the sensor without deleting the driver object.
- `read(value)` writes Celsius into a `float&` and returns `esp_err_t`.
- `celsius()` returns Celsius and panics on driver error.
- `milli()` returns milli-Celsius for integer telemetry.

Use this as a Core 0 guard rail when radio, CPU, DMA, and waveform peripherals are running hard.

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

`SeqReg` is cache-line aligned to avoid false sharing with adjacent state. `write()` masks OS-visible interrupts around the odd sequence window, and `read()` inserts a tiny `arc::pause()` between failed snapshots so a fast writer does not turn the losing core into a wasteful full-bus spin.

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

### `arc::net::Radio`

Shared Wi-Fi foundation for Core 0 transports.

- `base()` initializes typed NVS, `esp_netif`, the default event loop, the default STA netif, and the Wi-Fi driver exactly once.
- `prepare(mode, power_save)` sets storage, Wi-Fi mode, and power-save policy before a transport writes its own config.
- `start(mode, power_save)` starts Wi-Fi once and rejects later mode mismatches instead of silently reconfiguring a live radio.
- `sta()` returns the shared STA netif handle for transports that need IP state.
- `lease()` and `release()` let long-lived transports pin radio teardown while their event handlers are alive.
- `leases()` reports active transport pins.
- `stop()` stops Wi-Fi without throwing away the prepared configuration.
- `off()` deinitializes the Wi-Fi driver and destroys Arc-owned default STA netif state only when no transport leases are active.

Use `arc_requires(... net)` only when directly using `arc::net::Radio`. `udp` and `espnow` already include the same dependencies.

### `arc::net::Tcp`

Direct TCP client for Core 0 control and configuration paths.

- `dial(host, port, timeout_ms)` opens one socket and returns `arc::Result<arc::net::Tcp>`.
- `send(...)` and `recv(...)` return byte counts without hiding partial transfers.
- `send_all(...)` loops until the caller-owned buffer is fully written or an error occurs.
- `close()` and `native()` keep socket lifetime explicit.

Use this when a small protocol needs TCP but does not justify a framework-owned task.

### `arc::net::Http`

RAII wrapper for ESP-IDF HTTP client sessions.

- `make(config)` returns `arc::Result<arc::net::Http>`.
- `url(...)`, `method(...)`, `header(...)`, and `body(...)` set request fields.
- `perform()` covers the simple one-shot request path.
- `open(...)`, `write(...)`, `fetch()`, `read(...)`, and `close()` expose the streaming path.
- `status()`, `length()`, and `native()` expose response metadata and the raw handle.

Use this for Core 0 REST/config exchanges where HTTP should stay outside the realtime plane.

### `arc::net::Udp<Policy, Bus>`

Reusable Core 0 UDP transport plane.

UDP uses `arc::net::Radio` underneath and takes a radio lease while its Core 0 task is alive, so `Radio::off()` cannot destroy netif or Wi-Fi state beneath registered UDP event handlers.

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
- `arc::Sleep::causes()` and `arc::Sleep::woke(...)`
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
- `arc::SpiBus` and `arc::Spi` are the lanes to reach for when a sensor, display, or converter should move bytes through the SPI peripheral instead of CPU-owned toggling.
- `arc::I2s` is the right lane when the data is naturally framed and should live on a DMA-backed serial stream.
- `arc::Wave` is for deliberate CPU-owned edges, not for the common case of “just blink this periodically”.
- `arc::Reg` is better than a queue for latest-wins control words.
- For multi-field control words, prefer explicit fixed-width fields over C++ bitfields.
- `arc::Spsc` is better than a register when event history matters and ownership is one writer plus one reader.
- `arc::net::EspNow` is the transport to reach for before IP when you want radio latency and do not need routers.
- `arc::Ota` belongs on Core 0 with transports and storage, not in the realtime loop.
- `arc::Burst`, `arc::Trace`, and `arc::Count` are the first place to go when a pin-level job should move from CPU polling into dedicated hardware.
- `arc::Timer` is the right timebase when cycle counters are too core-local or when you need a true peripheral alarm.
- `arc::Mask` is for tiny deterministic sections, not for normal app structure.
- `arc::Tight` is the next step after `arc::App` when you need a masked step loop, not a normal forever-loop task body.
- `arc::SeqReg` is the right latest-snapshot lane once a control word no longer fits in `arc::Reg`.
- `arc::dmabuf`, `arc::cachebuf`, `arc::simdbuf`, `arc::ahbbuf`, and `arc::axibuf` are the right way to keep performance-critical heap placement explicit.
- `arc::dsp` is intentionally small: it is there to feed the compute plane, not to hide the math under a giant framework.
- UDP over Wi-Fi is a good first network demo, but it belongs under `examples/udp`, not in the root baseline.
- `app_main()` should remain the one C boundary; wrapping it further does not buy speed or clarity.
