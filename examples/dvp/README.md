# DVP Example

This is a standalone ESP-IDF project under `examples/dvp`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Dvp` to bind an ESP32-S3 LCD_CAM DVP input lane at compile time.
- It boots the camera controller and allocates one DMA-capable frame buffer.

## What It Shows

`arc::Dvp<arc::DvpLines<...>, vsync, pclk, href, xclk>` declares the parallel camera lane.

The example intentionally does not call `grab()` by default because a real DVP sensor must first be configured over its own control bus. After the sensor is configured and clocks are valid, the capture call is:

```cpp
std::size_t got = 0;
ESP_ERROR_CHECK(Cam::grab(frame, &got, 1000));
```

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_dvp.bin`
- `build/arc_dvp.elf`

## API Shape

```cpp
using Cam = arc::Dvp<
    arc::DvpLines<4, 5, 6, 7, 15, 16, 17, 18>,
    39, 40, 41, 42,
    160, 120>;

auto frame = Cam::buffer<std::uint16_t>(160U * 120U);
```
