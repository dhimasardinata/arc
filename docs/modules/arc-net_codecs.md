# `arc/net_codecs.hpp`

URI, streams, fixed records, CRDTs, MQTT, WebSocket, CoAP, and small HTTP server helpers without owning Wi-Fi.

## Fit

- Use it when a reader wants one coherent entry point for a domain or a subset build wants a profile root.
- Do not start here when one source file only needs a narrow peripheral or codec header.
- Verification focus: confirm the profile does not drag domain roots into small substrate builds unless that is intentional.

## Arc Contract

- Header: `arc/net_codecs.hpp`
- Module group: Profile Modules
- CMake feature: `net_codecs`
- Closest example: `.`

Declare `arc_requires(main_requires core net_codecs)` in the component that includes this header.

## CMake And Include

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core net_codecs)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

```cpp
#include "arc/net_codecs.hpp"
```

## Source Landmarks

Source landmarks: `arc/coap.hpp`, `arc/crdt.hpp`, `arc/http_server.hpp`, `arc/mqtt.hpp`, `arc/netrpc.hpp`, `arc/stream.hpp`, `arc/uri.hpp`, `arc/ws.hpp`.

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

The root project is the smallest place to try this module.

```sh
. ./env.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
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
