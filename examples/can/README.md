# CAN Example

This is a standalone ESP-IDF project under `examples/can`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Can` to bind the ESP32-S3 TWAI controller at compile time.
- It runs self-test loopback, so it can verify TX/RX without an external CAN transceiver.

## What It Shows

`arc::Can<tx, rx, bitrate>` owns one TWAI/CAN node.

The example:

- starts the CAN node on Core 0
- sends one classic 8-byte frame every second
- receives the looped-back frame through the ISR-backed SPSC ring
- logs `sent`, `done`, `rx`, `drop`, and `err` counters

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

- `build/arc_can.bin`
- `build/arc_can.elf`

## API Shape

```cpp
using Bus = arc::Can<4, 5, 500'000>;

Bus::start();
auto frame = Bus::frame(0x123, std::span<const std::uint8_t>{data, len});
ESP_ERROR_CHECK(Bus::send_wait(frame, 1000));

Bus::Frame rx{};
while (Bus::recv(rx)) {
    // consume rx.data[0..rx.bytes)
}
```

For real CAN bus usage, use separate TX/RX pins and an external CAN transceiver.
