# UDP Example

This is a standalone ESP-IDF project under `examples/udp`.

- Core 1 owns the waveform and telemetry production.
- Core 0 owns NVS, Wi-Fi station mode, DNS, and the UDP socket.
- Core 1 pushes ordered telemetry through `arc::Ring`.
- Core 0 pushes latest-wins commands back through `arc::Reg`.

The baseline root app stays transport-free on purpose. This example is where the network plane lives.

## Configure

From this directory:

```bash
. ./env.sh
idf.py set-target esp32s3
idf.py menuconfig
```

Set these fields in `Arc UDP`:

- `Wi-Fi SSID`
- `Wi-Fi password`
- `UDP host`
- `UDP port`

## Build And Flash

```bash
. ./env.sh
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_udp.bin`
- `build/arc_udp.elf`

## Receive Telemetry

On the receiving machine:

```bash
nc -ul 9000 | hexdump -C
```

Each UDP frame is 12 bytes, little-endian:

1. `tick` as `u32`
2. `seq` as `u32`
3. `level` as `u8`
4. `mark` as `u8`
5. padding as `u16`

## What It Does

- Core 1 toggles the LED through dedicated GPIO.
- Core 1 emits a telemetry frame on each output edge.
- Core 0 drains the ring and sends frames with `sendto()`.
- Core 0 toggles the control word every few seconds, changing rate, mark, and telemetry stride.
