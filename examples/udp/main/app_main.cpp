#include "arc.hpp"

#include "cfg.hpp"
#include "link.hpp"

using Led = arc::Dio<app::cfg::led, 0>;
using Bus = arc::Bus<app::Edge, app::Control, 256>;

struct Pulse {
    static void setup(Bus&) { Led::out(); Led::template write<false>(); }

    IRAM_ATTR static void run(Bus& bus) noexcept
    {
        std::uint32_t seq = 0;

        while (true) {
            const auto pace = static_cast<esp_cpu_cycle_count_t>(bus.pace.read());

            Led::template write<true>();
            emit(bus, seq, 1U);
            arc::fence();
            arc::Clock::spin(pace);

            Led::template write<false>();
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
        Bus& bus,
        std::uint32_t& seq,
        const std::uint8_t level) noexcept
    {
        const auto control = bus.control.read();
        const auto edge = seq++;
        if ((control.flags & app::trace_on) == 0U) {
            return;
        }
        if ((edge & mask(control.stride)) != 0U) {
            return;
        }

        static_cast<void>(bus.events.try_push(app::Edge{
            .tick = static_cast<std::uint32_t>(arc::Clock::tick()),
            .seq = edge,
            .level = level,
            .mark = control.mark,
        }));
    }
};

struct Udp {
    inline constexpr static char tag[] = "arc-udp";
    inline constexpr static std::uint32_t stack = app::cfg::net_stack;
    inline constexpr static TickType_t idle = pdMS_TO_TICKS(1);
    inline constexpr static TickType_t retry = pdMS_TO_TICKS(1000);
    inline constexpr static const char* ssid = app::cfg::ssid;
    inline constexpr static const char* pass = app::cfg::pass;
    inline constexpr static const char* host = app::cfg::host;
    inline constexpr static std::uint32_t port = app::cfg::port;

    static void start(Bus& bus) noexcept
    {
        last = xTaskGetTickCount();
        fast = false;
        app::boot(bus);
    }

    static void tick(Bus& bus, const TickType_t now) noexcept
    {
        if ((now - last) < pdMS_TO_TICKS(app::cfg::cmd_ms)) {
            return;
        }

        last = now;
        fast = !fast;

        const auto half_us = fast ? app::cfg::fast_half_us : app::cfg::half_us;
        const auto control = fast ? app::fast : app::slow;
        bus.pace.write(app::cycles(half_us));
        bus.control.write(control);

        ESP_LOGI(
            tag,
            "cmd half=%u us mark=%u stride=%u flags=0x%x",
            static_cast<unsigned>(half_us),
            static_cast<unsigned>(control.mark),
            static_cast<unsigned>(control.stride),
            static_cast<unsigned>(control.flags));
    }

private:
    inline static TickType_t last{};
    inline static bool fast{};
};

using Core0 = arc::net::Udp<Udp, Bus>;
using Core1 = arc::Plane<Pulse, app::cfg::stack, Bus>;

namespace app {

constinit inline Bus bus{};

inline void boot()
{
    app::boot(bus);
    Core0::boot(bus);
    Core1::boot(Udp::tag, bus);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
