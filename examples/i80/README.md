# I80 Example

This is a standalone ESP-IDF project under `examples/i80`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::I80Bus` and `arc::I80` to drive the ESP32-S3 LCD_CAM Intel 8080 path.
- The payload moves through the LCD/CAM + DMA path instead of a CPU bit-bang loop.

## What It Shows

`arc::I80Bus<arc::Lines<...>, dc, wr>` binds the parallel data bus at compile time.

`arc::I80<Bus, cs>` creates one panel IO endpoint and exposes:

- `param(cmd, data, bytes)` for command/parameter transfers
- `color(cmd, data, bytes)` for queued DMA color/data transfers
- `color_coherent(ticket, cmd, buffer)` for cache-safe queued payload ownership
- `buffer<T>(count)` for DMA-safe draw buffers
- `sent()`, `done()`, `idle()`, `ready(ticket)`, `wait()`, and `finish(ticket)` for the DMA queue state

The example repeatedly fills a tiny RGB565 buffer, queues it with command `0x2C` (`RAMWR` on common LCD controllers), and finishes that exact ticket before writing the buffer again. Adjust pins for your board before connecting hardware.

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

- `build/arc_i80.bin`
- `build/arc_i80.elf`

## API Shape

```cpp
using Bus = arc::I80Bus<arc::Lines<4, 5, 6, 7, 15, 16, 17, 18>, 21, 47>;
using Lcd = arc::I80<Bus, 48, 20'000'000>;

auto frame = Lcd::buffer<std::uint16_t>(128);
Lcd::Ticket paint{};
Lcd::color_coherent(paint, 0x2C, frame);
Lcd::finish(paint);
```
