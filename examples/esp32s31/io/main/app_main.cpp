#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/soc.hpp"
#include "arc/soc/target.hpp"

#if __has_include("esp_adc/adc_continuous.h") && __has_include("esp_adc/adc_oneshot.h")
#include "arc/adc.hpp"
#define ARC_S31_KORVO_IO_DRIVER_CONTRACT 1
#else
#define ARC_S31_KORVO_IO_DRIVER_CONTRACT 0
#endif

#if __has_include("driver/rmt_tx.h") && __has_include("driver/rmt_encoder.h")
#include "arc/burst.hpp"
#define ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT 1
#else
#define ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT 0
#endif

namespace {

using Board = arc::board::Korvo1;

#if ARC_S31_KORVO_IO_DRIVER_CONTRACT
using ButtonPad = arc::Adc<Board::Button::adc>;
using ButtonBus = arc::AdcBus<>;
using Button = arc::AdcOne<ButtonBus, ButtonPad, false>;

static_assert(ButtonPad::io() == Board::Button::adc);
static_assert(Button::io() == Board::Button::adc);
static_assert(sizeof(ButtonBus::Resource) > 0U);
static_assert(sizeof(Button::Resource) > 0U);
#endif

#if ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT
inline constexpr std::uint32_t status_led_resolution_hz = 10'000'000U;

using StatusLed = arc::Burst<Board::Led::ws2812, status_led_resolution_hz, 48, 1, false>;

inline constexpr auto status_led_zero = StatusLed::symbol(4U, true, 9U, false);
inline constexpr auto status_led_one = StatusLed::symbol(8U, true, 5U, false);

[[nodiscard]] consteval std::array<rmt_symbol_word_t, 24> status_frame(
    const std::uint32_t grb) noexcept
{
    std::array<rmt_symbol_word_t, 24> out{};
    for (std::size_t bit = 0; bit < out.size(); ++bit) {
        const auto mask = std::uint32_t{1} << (23U - static_cast<std::uint32_t>(bit));
        out[bit] = (grb & mask) != 0U ? status_led_one : status_led_zero;
    }
    return out;
}

inline constexpr auto status_led_probe = status_frame(0x000101U);

static_assert(status_led_zero.val != status_led_one.val);
static_assert(status_led_probe.size() == 24U);
static_assert(status_led_probe.front().val == status_led_zero.val);
static_assert(status_led_probe.back().val == status_led_one.val);
static_assert(sizeof(StatusLed::Resource) > 0U);
#endif

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "io requires ARC_TARGET=esp32s31");
    static_assert(arc::Soc::adc && arc::Soc::rmt, "ESP32-S31 IO scaffold expects SDK ADC/RMT capabilities");
    static_assert(arc::Topology<Board>);
    static_assert(Board::Onboard::button, "Korvo button fact changed");
    static_assert(Board::Onboard::status_led, "Korvo status LED fact changed");
    static_assert(Board::Button::adc == 42, "Korvo button ADC pin changed");
    static_assert(std::string_view{Board::Button::signal} == "ADC BUTTON", "Korvo button signal changed");
    static_assert(Board::Button::count == 4, "Korvo function button count changed");
    static_assert(Board::Button::play == 0 && Board::Button::set == 1, "Korvo function button order changed");
    static_assert(Board::Button::vol_down == 2 && Board::Button::vol_up == 3, "Korvo volume button order changed");
    static_assert(Board::Button::shared_adc, "Korvo function buttons no longer share the ADC lane");
    static_assert(Board::Button::ui_control && Board::Button::audio_test, "Korvo function button role changed");
    static_assert(Board::Led::ws2812 == 37, "Korvo status LED pin changed");
    static_assert(std::string_view{Board::Led::signal} == "WS2812_CTRL", "Korvo status LED signal changed");
    static_assert(Board::Led::count == 1U && Board::Led::rgb, "Korvo status LED shape changed");
    static_assert(Board::Led::addressable, "Korvo status LED protocol changed");
    static_assert(Board::Led::ws2812 == Board::Strap::status_led, "Korvo status LED strap mapping changed");
    static_assert(sizeof(Board::Resource::ButtonAdc) > 0U);
    static_assert(sizeof(Board::Resource::StatusLed) > 0U);

    std::printf(
        "arc-s31-io target=%s board=%s arch=%s button_adc=%d button_signal=%s buttons=%d button_ui=%d button_audio=%d status_led=%d led_signal=%s led_count=%u led_rgb=%d led_addr=%d led_strap=%d driver_contract=%d rmt_contract=%d status_symbols=%u ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        Board::Button::adc,
        Board::Button::signal,
        Board::Button::count,
        Board::Button::ui_control ? 1 : 0,
        Board::Button::audio_test ? 1 : 0,
        Board::Led::ws2812,
        Board::Led::signal,
        static_cast<unsigned>(Board::Led::count),
        Board::Led::rgb ? 1 : 0,
        Board::Led::addressable ? 1 : 0,
        Board::Strap::status_led,
        ARC_S31_KORVO_IO_DRIVER_CONTRACT,
        ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT,
#if ARC_S31_KORVO_STATUS_LED_RMT_CONTRACT
        static_cast<unsigned>(status_led_probe.size()),
#else
        0U,
#endif
        Board::Onboard::button && Board::Onboard::status_led ? 1 : 0);
}
