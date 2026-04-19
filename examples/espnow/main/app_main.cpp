#include <array>
#include <cstdint>

#include "arc.hpp"
#include "arc/espnow.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

namespace app {

inline constexpr char tag[] = "arc-now";
inline constexpr int led = CONFIG_ARC_ESPNOW_LED;
inline constexpr std::uint32_t stack = CONFIG_ARC_ESPNOW_STACK;
inline constexpr std::uint32_t net_stack = CONFIG_ARC_ESPNOW_NET_STACK;
inline constexpr std::uint32_t half_us = CONFIG_ARC_ESPNOW_HALF_US;
inline constexpr std::uint32_t fast_half_us = CONFIG_ARC_ESPNOW_FAST_HALF_US;
inline constexpr std::uint32_t cmd_ms = CONFIG_ARC_ESPNOW_CMD_MS;
inline constexpr std::uint8_t channel = static_cast<std::uint8_t>(CONFIG_ARC_ESPNOW_CHANNEL);
inline constexpr std::uint8_t trace_on = 0x01U;

struct Edge {
    std::uint32_t tick;
    std::uint32_t seq;
    std::uint8_t level;
    std::uint8_t mark;
    std::uint16_t pad;
};

struct Control {
    std::uint8_t mark;
    std::uint8_t stride;
    std::uint8_t flags;
    std::uint8_t pad;
};

static_assert(sizeof(Edge) == 12U);
static_assert(sizeof(Control) == 4U);

inline constexpr Control slow{
    .mark = 0x31U,
    .stride = 0U,
    .flags = trace_on,
    .pad = 0U,
};

inline constexpr Control fast{
    .mark = 0x62U,
    .stride = 1U,
    .flags = trace_on,
    .pad = 0U,
};

[[nodiscard]] constexpr std::uint32_t cycles(const std::uint32_t us) noexcept
{
    return us * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
}

using Led = arc::Drive<led, 0>;
using Link = arc::Link<Edge, Control, 256>;

inline void prime(Link& bus)
{
    bus.pace.write(cycles(half_us));
    bus.control.write(slow);
}

struct Pulse {
    static void setup(Link&) { Led::out(); Led::off(); }

    IRAM_ATTR static void run(Link& bus) noexcept
    {
        std::uint32_t seq = 0;

        while (true) {
            const auto pace = static_cast<esp_cpu_cycle_count_t>(bus.pace.read());

            Led::on();
            emit(bus, seq, 1U);
            arc::fence();
            arc::Clock::spin(pace);

            Led::off();
            emit(bus, seq, 0U);
            arc::fence();
            arc::Clock::spin(pace);
        }
    }

private:
    [[nodiscard]] IRAM_ATTR [[gnu::always_inline]] static inline std::uint32_t mask(
        const std::uint8_t stride) noexcept
    {
        const auto shift = stride >= 31U ? 31U : stride;
        return shift == 0U ? 0U : ((std::uint32_t{1} << shift) - 1U);
    }

    IRAM_ATTR [[gnu::noinline]] static void emit(
        Link& bus,
        std::uint32_t& seq,
        const std::uint8_t level) noexcept
    {
        const auto control = bus.control.read();
        const auto edge = seq++;
        if ((control.flags & trace_on) == 0U) {
            return;
        }
        if ((edge & mask(control.stride)) != 0U) {
            return;
        }

        static_cast<void>(bus.events.try_push(Edge{
            .tick = static_cast<std::uint32_t>(arc::Clock::tick()),
            .seq = edge,
            .level = level,
            .mark = control.mark,
            .pad = 0U,
        }));
    }
};

struct Radio {
    inline constexpr static char tag[] = "arc-now";
    inline constexpr static std::uint32_t stack = net_stack;
    inline constexpr static TickType_t idle = pdMS_TO_TICKS(1);
    inline constexpr static std::uint8_t channel = app::channel;
    inline constexpr static std::array<std::uint8_t, 6> peer{
        0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU,
    };

    static void start(Link& bus) noexcept
    {
        last = xTaskGetTickCount();
        fast_mode = false;
        prime(bus);
    }

    static void tick(Link& bus, const TickType_t now) noexcept
    {
        if ((now - last) < pdMS_TO_TICKS(cmd_ms)) {
            return;
        }

        last = now;
        fast_mode = !fast_mode;

        const auto period = fast_mode ? fast_half_us : half_us;
        const auto control = fast_mode ? fast : slow;
        bus.pace.write(cycles(period));
        bus.control.write(control);

        ESP_LOGI(
            tag,
            "cmd half=%u us mark=0x%02x stride=%u flags=0x%x channel=%u",
            static_cast<unsigned>(period),
            static_cast<unsigned>(control.mark),
            static_cast<unsigned>(control.stride),
            static_cast<unsigned>(control.flags),
            static_cast<unsigned>(channel));
    }

private:
    inline static TickType_t last{};
    inline static bool fast_mode{};
};

using Core0 = arc::net::EspNow<Radio, Link>;
using Core1 = arc::Plane<Pulse, stack, Link>;

constinit inline Link bus{};

inline void boot()
{
    prime(bus);
    Core0::boot(bus);
    Core1::boot(Radio::tag, bus);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
