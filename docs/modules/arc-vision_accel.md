# `arc/vision_accel.hpp`

PPA/JPEG/H264 frame and bitstream plan validation.

## Fit

- Use it when ESP32-P4 PPA, JPEG, or H264 paths need fixed frame and bitstream plans before board policy submits work.
- Do not start here when SDK handle lifetime, queue depth, interrupts, or cache maintenance policy are still undefined.
- Verification focus: prove input/output sizes and policy calls on host, then benchmark on ESP32-P4 before publishing timing claims.

## Arc Contract

- Header: `arc/vision_accel.hpp`
- Module group: DSP, Control, ML, And Vision
- CMake feature: `vision_accel`
- Closest example: `examples/esp32s3/dvp`

Declare `arc_requires(main_requires core vision_accel)` in the component that includes this header.

## CMake And Include

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core vision_accel)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

```cpp
#include "arc/vision_accel.hpp"
```

## Source Landmarks

Source landmarks: `Rect`, `InFrame`, `OutFrame`, `PpaSrm`, `PpaFill`, `PpaBlend`, `JpegEncode`, `H264Encode`, `JpegEncoder`, `H264Encoder`.

## Start From Zero

- Start from the closest example or the root project listed below.
- Load the ESP-IDF environment with `. ./env.sh`.
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

The closest shipped example is `examples/esp32s3/dvp`.

```sh
. ./env.sh
idf.py -C examples/esp32s3/dvp build
idf.py -C examples/esp32s3/dvp -p /dev/ttyACM0 flash monitor
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
