# `arc/ulp_ml.hpp`

ULP-side int8 dense inference and semantic/audio wake helpers.

## Fit

- Use it when tiny retained-state policy must keep running while the main cores sleep.
- Do not start here when the job needs heap, Wi-Fi, storage, or broad C++ runtime support.
- Verification focus: verify RTC memory layout, wake source, retained data alignment, and the main-core handoff after wake.

## Arc Contract

- Header: `arc/ulp_ml.hpp`
- Module group: ULP And Low-Power Coprocessor
- CMake feature: `ulp_ml`
- Closest example: `examples/esp32s3/sleep`

Declare `arc_requires(main_requires core ulp_ml)` in the component that includes this header.

## CMake And Include

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core ulp_ml)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

```cpp
#include "arc/ulp_ml.hpp"
```

## Source Landmarks

Source landmarks: `QuantParams`, `QuantDenseS8`, `AudioSignature`, `AudioSignatureWake`, `SemanticWake`, `Model`.

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

The closest shipped example is `examples/esp32s3/sleep`.

```sh
. ./env.sh
idf.py -C examples/esp32s3/sleep build
idf.py -C examples/esp32s3/sleep -p /dev/ttyACM0 flash monitor
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
