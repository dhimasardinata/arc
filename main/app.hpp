#pragma once

#include <cstdint>

#include "esp_attr.h"
#include "esp_log.h"

#include "arc.hpp"

namespace app {

using Led = arc::Dio<arc::cfg::led, 0>;
using Sense = arc::Din<arc::cfg::sense, 0>;

struct Telemetry {
    std::uint32_t tick;
    std::uint32_t seq;
    std::uint8_t level;
};

struct Ctx {
    arc::Ring<Telemetry, 256> tx{};
    arc::Reg<std::uint32_t> half{};
};

struct Loop {
    static void setup(Ctx&) { Sense::in(); Led::out(); Led::template write<false>(); }

    IRAM_ATTR static void run(Ctx& ctx) noexcept
    {
        auto last = Sense::high();
        std::uint32_t seq = 0;

        while (true) {
            const auto half_cycles = static_cast<esp_cpu_cycle_count_t>(ctx.half.read());
            step(ctx, last, seq);
            Led::template write<true>();
            arc::fence();
            arc::Clock::spin(half_cycles);

            step(ctx, last, seq);
            Led::template write<false>();
            arc::fence();
            arc::Clock::spin(half_cycles);
        }
    }

private:
    IRAM_ATTR [[gnu::noinline]] static void step(
        Ctx& ctx,
        bool& last,
        std::uint32_t& seq) noexcept
    {
        const auto level = Sense::high();
        if (level != last) {
            last = level;
            static_cast<void>(ctx.tx.try_push(Telemetry{
                .tick = static_cast<std::uint32_t>(arc::Clock::tick()),
                .seq = seq++,
                .level = static_cast<std::uint8_t>(level),
            }));
        }
    }
};

using Rt = arc::Plane<Loop, arc::cfg::stack, Ctx>;

inline constexpr char tag[] = "arc";
inline constexpr TickType_t cmd_ticks = pdMS_TO_TICKS(2000);
inline constexpr std::uint32_t fast_half_us = 50000;

#if CONFIG_LOG_DEFAULT_LEVEL > 0
inline constexpr TickType_t poll_ticks = pdMS_TO_TICKS(1);
#else
inline constexpr TickType_t poll_ticks = pdMS_TO_TICKS(50);
#endif

constinit inline Ctx ctx{};
constinit inline arc::TaskMem<4096> io_mem{};

[[nodiscard]] constexpr std::uint32_t half_cycles(const std::uint32_t half_us) noexcept
{
    return static_cast<std::uint32_t>(arc::Clock::us(half_us, arc::cfg::mhz));
}

inline void drain() noexcept
{
#if CONFIG_LOG_DEFAULT_LEVEL > 0
    Telemetry sample{};
    while (ctx.tx.try_pop(sample)) {
        ESP_LOGI(
            tag,
            "edge seq=%u level=%u tick=%u",
            static_cast<unsigned>(sample.seq),
            static_cast<unsigned>(sample.level),
            static_cast<unsigned>(sample.tick));
    }
#endif
}

inline void io([[maybe_unused]] void* raw) noexcept
{
    auto last_cmd = xTaskGetTickCount();
    auto fast = false;

    for (;;) {
        drain();

        const auto now = xTaskGetTickCount();
        if ((now - last_cmd) >= cmd_ticks) {
            last_cmd = now;
            fast = !fast;

            const auto half_us = fast ? fast_half_us : arc::cfg::half_us;
            ctx.half.write(half_cycles(half_us));
            ESP_LOGI(tag, "cmd half=%u us", static_cast<unsigned>(half_us));
        }

        vTaskDelay(poll_ticks);
    }
}

inline void boot()
{
    ctx.half.write(half_cycles(arc::cfg::half_us));

    const auto io_task = arc::spawn(
        &io,
        "io",
        nullptr,
        2,
        arc::Core::core0,
        io_mem);

    configASSERT(io_task != nullptr);
    Rt::boot(tag, ctx);
}

}  // namespace app
