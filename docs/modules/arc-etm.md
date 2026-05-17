# `arc/etm.hpp`

Event Task Matrix event/task/channel routing.

## What It Owns

- Header: `arc/etm.hpp`
- Module group: GPIO, Timing, And Power
- CMake feature: `etm`
- Closest example: `examples/esp32s3/pwm`

Declare the CMake feature with `arc_requires(main_requires core etm)` when this header needs ESP-IDF components beyond the base Arc component.

## Start From Zero

1. Create or clone an Arc project.
2. Load the ESP-IDF environment with `. ./env.sh`.
3. Include the module header in the file that owns this hardware or policy lane.
4. Add the matching `arc_requires(...)` feature in that component's `CMakeLists.txt`.
5. Build the closest example first, then move the same pattern into your app.

Minimal component shape:

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core etm)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

```cpp
#include "arc/etm.hpp"

namespace app {
void boot()
{
    // Keep board policy explicit here. Configure pins, buffers, and ownership
    // before handing work to Core 1 or a Core 0 transport task.
}
}

extern "C" void app_main()
{
    app::boot();
}
```

## Usage Pattern

- Put topology facts in one board header instead of scattering aliases.
- Keep buffer ownership visible with `std::span`, `std::array`, or Arc capability buffers.
- Use recoverable `init()`, `begin()`, or `set(...)` paths when runtime data can fail.
- Use fail-fast `boot()` or `start()` only for board invariants.
- Keep slow logs, storage, Wi-Fi, and protocol work on Core 0.

## Step-By-Step Check

1. Decide whether this module owns silicon, memory, protocol bytes, or policy only.
2. Name the owner type once, close to the board topology.
3. Allocate any DMA or shared buffers before the hardware starts.
4. Initialize with the recoverable path while bringing up the board.
5. Switch to the fail-fast path only after the topology is treated as fixed.
6. Log from Core 0 after the hot path has handed off a compact event or snapshot.

## Example

The closest shipped example is `examples/esp32s3/pwm`.

```sh
. ./env.sh
idf.py -C examples/esp32s3/pwm build
idf.py -C examples/esp32s3/pwm -p /dev/ttyACM0 flash monitor
```

If the example needs a jumper, sensor, display, or external bus device, read the
example README before flashing. The command above proves the build path; real
electrical output still depends on the attached board.

## Simulated Output

This is a documentation simulation, not a captured hardware log:

```text
I (000) arc-doc: module=arc/etm
I (001) arc-doc: feature=etm example=pwm
I (002) arc-doc: init policy explicit
I (003) arc-doc: no hidden heap in hot path
```

## Next Reading

- [Module Guide](/modules)
- [API Reference](/api)
- [Examples](/examples)
