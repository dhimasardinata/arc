#include <array>
#include <cstdint>

#include "arc.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-dsp";
inline constexpr std::uint32_t stack = 6144;
inline constexpr TickType_t log_ticks = pdMS_TO_TICKS(1000);
inline constexpr std::size_t lanes = 64;

struct Stats {
    float dot;
    float tail;
    float peak;
    std::uint32_t turns;
};

constinit static arc::SeqReg<Stats> stats{};
constinit static arc::TaskMem<stack> host_mem{};

static arc::CapsBuf<float> a{};
static arc::CapsBuf<float> b{};
static arc::CapsBuf<float> mix{};

struct Kernel {
    static void setup()
    {
        a = arc::simdbuf<float>(lanes);
        b = arc::simdbuf<float>(lanes);
        mix = arc::simdbuf<float>(lanes);

        configASSERT(static_cast<bool>(a));
        configASSERT(static_cast<bool>(b));
        configASSERT(static_cast<bool>(mix));

        for (std::size_t i = 0; i < lanes; ++i) {
            a[i] = static_cast<float>(i) * 0.25F;
            b[i] = static_cast<float>(lanes - i) * 0.125F;
            mix[i] = 0.0F;
        }
    }

    IRAM_ATTR static void loop() noexcept
    {
        static arc::dsp::Fir<float, 8>::State fir{};
        static constexpr arc::dsp::Fir<float, 8>::Coeffs taps{
            0.05F, 0.08F, 0.12F, 0.15F, 0.15F, 0.12F, 0.08F, 0.05F};
        static std::uint32_t turns = 0U;

        arc::dsp::scale(mix.data(), a.data(), 0.50F, lanes);
        arc::dsp::mac(mix.data(), b.data(), 0.25F, lanes);

        float tail = 0.0F;
        for (std::size_t i = 0; i < lanes; ++i) {
            tail = arc::dsp::Fir<float, 8>::step(fir, taps, mix[i]);
        }

        stats.write(Stats{
            .dot = arc::dsp::dot(a.data(), b.data(), lanes),
            .tail = tail,
            .peak = arc::dsp::peak(mix.data(), lanes),
            .turns = ++turns,
        });
    }
};

using Core1 = arc::App<Kernel, arc::cfg::stack, arc::Core::core1>;

void host(void*) noexcept
{
    while (true) {
        const auto snap = stats.read();
        ESP_LOGI(
            tag,
            "dot=%d.%03d tail=%d.%03d peak=%d.%03d turns=%u lanes=%u",
            static_cast<int>(snap.dot),
            static_cast<int>((snap.dot - static_cast<int>(snap.dot)) * 1000.0F),
            static_cast<int>(snap.tail),
            static_cast<int>((snap.tail - static_cast<int>(snap.tail)) * 1000.0F),
            static_cast<int>(snap.peak),
            static_cast<int>((snap.peak - static_cast<int>(snap.peak)) * 1000.0F),
            static_cast<unsigned>(snap.turns),
            static_cast<unsigned>(lanes));
        vTaskDelay(log_ticks);
    }
}

inline void boot()
{
    const auto handle = arc::spawn(
        &host,
        "dsp",
        nullptr,
        1,
        arc::Core::core0,
        host_mem);
    configASSERT(handle != nullptr);

    Core1::boot(tag);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
