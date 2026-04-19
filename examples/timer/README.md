# Timer Example

This is a standalone ESP-IDF project under `examples/timer`.

- The program is written directly in `main/app_main.cpp`.
- `arc::Timer` supplies a 1 MHz hardware timebase.
- A GPTimer alarm ISR toggles a dedicated GPIO through `arc::Drive`.
- A tiny host task logs the free-running timer count so you can confirm the hardware clock is live.

## Build And Run

From this directory:

```bash
. ./env.sh
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

For fish:

```fish
source ./env.fish
idf.py menuconfig
idf.py build
idf.py size
idf.py -p /dev/ttyACM0 flash monitor
```

Binary outputs:

- `build/arc_timer.bin`
- `build/arc_timer.elf`

## API Shape

```cpp
using Led = arc::Drive<CONFIG_ARC_TIMER_LED, 0, arc::Core::core0>;
using Tick = arc::Timer<1'000'000>;
```

The timer alarm is bound at compile time:

```cpp
struct Beat {
    IRAM_ATTR static bool alarm(const gptimer_alarm_event_data_t&) noexcept
    {
        Led::toggle();
        return false;
    }
};

Tick::start<Beat>();
Tick::alarm(period, 0, true);
```

This keeps the periodic edge generation on the timer hardware and ISR path instead of a busy loop.
