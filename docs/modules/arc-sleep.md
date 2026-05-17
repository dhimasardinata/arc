# `arc/sleep.hpp`

Light/deep sleep entry and wake-source policy.

## Fit

- Use it when pin edges, timing sources, sleep state, watchdogs, or power locks must be explicit at the call site.
- Do not start here when a slow policy task can own the work without a realtime hardware contract.
- Verification focus: verify target clock source, interrupt context, wake source, and board-level pin safety before flashing.

## Arc Contract

- Header: `arc/sleep.hpp`
- Module group: GPIO, Timing, And Power
- CMake feature: `core`
- Closest example: `examples/esp32s3/sleep`

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
#include "arc/sleep.hpp"
```

## Source Landmarks

Source landmarks: `Sleep`.

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
