# Arc ESP32-S31 Examples

Experimental ESP32-S31 scaffolds for the ESP32-S31-Korvo-1 board. These
projects are skipped by default CI until the pinned ESP-IDF checkout exposes a
usable `esp32s31` target.

Build an example only when preview ESP-IDF target support is available:

```sh
export ARC_IDF_PATH=/path/to/preview-esp-idf
python3 tools/s31-readiness.py --idf-path "$ARC_IDF_PATH" --require-sdk --format report
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio
python3 tools/s31-build.py --list-ports
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor --dry-run
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --port /dev/ttyACM0 --monitor
python3 tools/s31-build.py --idf-path "$ARC_IDF_PATH" --example audio --auto-port --monitor --dry-run
```

Omit `--example` to preflight and build every ESP32-S31 example in the manifest. Use `--list-ports` before flashing a physical Korvo board. Flashing or monitoring requires exactly one `--example` and either `--port` or `--auto-port`; `--monitor` implies `flash`. Use `--auto-port` only when exactly one Korvo serial port is connected; otherwise pass `--port` explicitly.

The shared board facts live in `components/arc/include/arc/board/esp32s31_korvo.hpp`
as `arc::board::Korvo1`. The header carries the Korvo module, wireless, setup,
power, download, audio, I2C, LCD, SD, camera, UART0, button, LED, USB, and
strapping pin groups plus a single `arc::Pins` pack for topology checks.
`Korvo1::Resource` provides the board-level ownership claims that examples bind
back to their guarded driver aliases. The `Korvo1*Graph` aliases expose codec,
audio, LCD, SD, optional SPI NAND, camera, console, and boot-strapping signal
routes without requiring peripheral driver headers.

`Korvo1::Usb::dp_module_pin` and `dm_module_pin` are ESP32-S31-WROOM-3 module
pins, not GPIO numbers. The board topology keeps their GPIO sentinels at `-1`
so USB_DP/USB_DM cannot be confused with GPIO40/GPIO41 LCD and strapping lanes.

All ESP32-S31 examples include `examples/esp32s31/sdkconfig.defaults`, which
points each nested project back to the root 16 MB partition table and uses
PSRAM auto-detect instead of the ESP32-S3 fixed 8 MB PSRAM type.

| Example | Intended surface |
| --- | --- |
| `amp` | Korvo-bound Core0/Core1 migration policy scaffolding; true bare-core AMP remains off until an S31 policy exists. |
| `audio` | Korvo audio codec pins, one/two speaker setup facts, dual analog microphone speech/wake facts, two NS4150B PA chips, 2 mm speaker connector pitch, I2C control pins, PA enable GPIO, isolated audio power, and DMA ownership. |
| `cam` | Korvo camera, 3.3 V to 2.8 V/1.5 V LDO rail facts, JPEG streaming, and 800x480 RGB565 LCD bus topology with DMA ownership. |
| `console` | Korvo UART0 console topology, two data-capable USB cable setup facts, USB-C UART power/flash bridge, manual/automatic download facts, and guarded UART contract. |
| `control` | Korvo-bound deterministic control-loop placement on the S31 core map. |
| `io` | Korvo GPIO42 `ADC BUTTON` shared function buttons and GPIO37 `WS2812_CTRL` addressable RGB status LED RMT ownership facts. |
| `lcd` | Korvo ST7262E43/GT1151 4.3-inch 800x480 RGB565 LCD topology, strap-overlapped control pins, and DMA frame ownership. |
| `ml` | Korvo-bound RISC-V/SIMD-aware fixed-shape ML helpers. |
| `ptp` | PTP timing scaffold; Korvo needs an external Ethernet PHY path for hardware timestamping. |
| `radio` | Korvo ESP32-S31-WROOM-3 Wi-Fi 6, Bluetooth 5.4, Bluetooth Classic, BLE, IEEE 802.15.4, Zigbee 3.0, Thread 1.4, PCB antenna routing, and guarded Arc ESP-NOW/BLE Mesh/Thread contracts. |
| `sd` | Korvo onboard SDIO 3.0 4-bit microSD topology for audio storage/playback, shared SPI NAND rework/voltage part counts, strap-overlapped control GPIO, and guarded SDMMC mount contract. |
| `security` | ESP32-S31 secure boot, flash encryption, TEE, PUF, WorldGuard, and Arc SecureBoot/WorldGuard/PUF/Cloak contract scaffolding without eFuse or flash-encryption side effects. |
| `usb` | Korvo USB OTG PHY ownership, Type-A 500 mA host budget, TPS2051C current-limited switch facts, 3 A high-load input envelope, and USB descriptor scaffolding. |
