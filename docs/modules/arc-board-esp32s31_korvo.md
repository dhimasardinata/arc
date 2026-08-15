# `arc/board/esp32s31_korvo.hpp`

ESP32-S31-Korvo-1 board pin facts and topology guards.

## Fit

- Use it when firmware structure, task lifetime, ownership, command parsing, or policy state needs a visible contract.
- Do not start here when a hardware-specific module already owns the same decision more directly.
- Verification focus: put the owner near board topology, make rollback explicit, and keep slow side effects on Core 0.

## Arc Contract

- Header: `arc/board/esp32s31_korvo.hpp`
- Module group: Program Shape And Ownership
- CMake feature: `core`
- Closest example: `examples/esp32s31/audio`

Declare `arc_requires(main_requires core)` in the component that includes this header.

## CMake And Include

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

```cpp
#include "arc/board/esp32s31_korvo.hpp"
```

## Source Landmarks

Source landmarks: `Korvo1`, `Module`, `Wireless`, `Onboard`, `Audio`, `AudioCodec`, `I2c`, `Lcd`, `Display`, `Sd`, `SpiNand`, `Cam`, `CamModule`, `Uart0`, `ConsoleBridge`, `Download`, `Button`, `Led`, `Usb`, `UsbHost`, `Power`, `Setup`, `Strap`, `Resource`, `AudioBus`, `AudioPa`, `CodecI2c`, `LcdBus`, `SdSlot`, `SdCtrl`, `SpiNandBus`, `CamBus`, `ConsoleUart`, `ButtonAdc`, `StatusLed`, `UsbOtg`, `Korvo1Signal`, `Korvo1CodecGraph`, `Korvo1AudioGraph`, `Korvo1LcdGraph`, `Korvo1SdGraph`, `Korvo1NandGraph`, `Korvo1CamGraph`, `Korvo1ConsoleGraph`, `Korvo1StrapGraph`.

## Start From Zero

- Start from the closest example or the root project listed below.
- Set `ARC_IDF_PATH` to an ESP32-S31 preview ESP-IDF checkout, run the S31 readiness preflight, then use `tools/s31-build.py` for the selected Korvo example.
- Add the include and CMake feature only in the component that owns this lane.
- Keep board topology, buffers, and ownership in one visible owner type.
- Move from build proof to hardware proof only after the wiring or runtime dependency is known.

## Owner Skeleton

```cpp
namespace app {
void boot()
{
    // Put board policy, buffer ownership, and failure handling here.
    // Keep Core 1 hot work separate from Core 0 service work.
}
}

extern "C" void app_main()
{
    app::boot();
}
```

## Step-By-Step Check

1. Decide whether this module owns silicon, memory, protocol bytes, or policy only.
2. Name the owner type once, close to the board topology.
3. Allocate any DMA or shared buffers before the hardware starts.
4. Initialize with the recoverable path while bringing up the board.
5. Switch to the fail-fast path only after the topology is treated as fixed.
6. Log from Core 0 after the hot path has handed off a compact event or snapshot.

## Build Or Example

The closest shipped example is `examples/esp32s31/audio`.

```sh
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor
```

## Runtime Check

The build command proves the dependency path. Runtime proof still needs the
actual board condition that matches this module: attached device, loopback,
radio peer, flash partition, sleep wake source, or captured serial/network
output. Do not turn the example command into a performance or hardware claim
without that evidence.

## Next Reading

- [Module Guide](/modules)
- [API Reference](/api)
- [Examples](/examples)
