# PWM Example

This is a standalone ESP-IDF project under `examples/pwm`.

- The program is written directly in `main/app_main.cpp`.
- It uses `arc::Pwm` to configure LEDC once and then lets hardware own the waveform.
- No pinned task, no busy loop, and no Core 1 spin is needed for a simple periodic output.
- Defaults target `ESP32-S3 N16R8` with `16 MB` flash and `8 MB` Octal PSRAM.

## What It Shows

At runtime, this example does one thing:

- route a compile-time LED pin to LEDC
- configure a compile-time PWM frequency and duty
- start hardware PWM and return from `app_main()`

This is the right pattern when the waveform is periodic and the CPU should do nothing after setup.

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

- `build/arc_pwm.bin`
- `build/arc_pwm.elf`

## API Shape

The example keeps the program surface tiny:

```cpp
using Led = arc::Pwm<CONFIG_ARC_PWM_LED, CONFIG_ARC_PWM_HZ, CONFIG_ARC_PWM_DUTY>;

namespace app {
inline void boot() { Led::start(); }
}
```

The main API pieces are:

- `arc::Pwm<Pin, Hz, DutyPermille, Channel, Timer, Bits>`
- `arc::Pwm::start()`
- `arc::Pwm::on()`
- `arc::Pwm::off()`
- `arc::Pwm::pause()`
- `arc::Pwm::resume()`
- `arc::Pwm::duty<permille>()`
