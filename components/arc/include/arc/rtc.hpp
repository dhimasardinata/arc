#pragma once

#include <cstdint>

#include "arc/claim.hpp"
#include "arc/result.hpp"

#include "driver/rtc_io.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "hal/rtc_io_types.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

static_assert(SOC_RTCIO_PIN_COUNT > 0, "Arc RTC GPIO requires RTC IO pads");
static_assert(SOC_RTCIO_INPUT_OUTPUT_SUPPORTED, "Arc RTC GPIO requires RTC IO input/output support");
static_assert(SOC_RTCIO_HOLD_SUPPORTED, "Arc RTC GPIO requires RTC IO hold support");
static_assert(SOC_RTCIO_WAKE_SUPPORTED, "Arc RTC GPIO requires RTC IO wake support");

struct RtcGpio {
    static constexpr std::uint32_t count = SOC_RTCIO_PIN_COUNT;

    [[nodiscard]] static bool valid(const gpio_num_t pin) noexcept
    {
        return rtc_gpio_is_valid_gpio(pin);
    }

    [[nodiscard]] static Result<int> index(const gpio_num_t pin) noexcept
    {
        const int out = rtc_io_number_get(pin);
        if (out < 0) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return ok(out);
    }

    [[nodiscard]] static Status init(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_init(pin));
    }

    [[nodiscard]] static Status deinit(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_deinit(pin));
    }

    [[nodiscard]] static Status mode(const gpio_num_t pin, const rtc_gpio_mode_t mode) noexcept
    {
        return status(rtc_gpio_set_direction(pin, mode));
    }

    [[nodiscard]] static Status sleep_mode(const gpio_num_t pin, const rtc_gpio_mode_t mode) noexcept
    {
        return status(rtc_gpio_set_direction_in_sleep(pin, mode));
    }

    [[nodiscard]] static Result<bool> level(const gpio_num_t pin) noexcept
    {
        const auto raw = rtc_gpio_get_level(pin);
        if (raw > 1U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return ok(raw != 0U);
    }

    [[nodiscard]] static Status write(const gpio_num_t pin, const bool high) noexcept
    {
        return status(rtc_gpio_set_level(pin, high ? 1U : 0U));
    }

    [[nodiscard]] static Status pullup(const gpio_num_t pin, const bool enable = true) noexcept
    {
        return status(enable ? rtc_gpio_pullup_en(pin) : rtc_gpio_pullup_dis(pin));
    }

    [[nodiscard]] static Status pulldown(const gpio_num_t pin, const bool enable = true) noexcept
    {
        return status(enable ? rtc_gpio_pulldown_en(pin) : rtc_gpio_pulldown_dis(pin));
    }

    [[nodiscard]] static Status floating(const gpio_num_t pin) noexcept
    {
        if (auto err = rtc_gpio_pullup_dis(pin); err != ESP_OK) {
            return fail(err);
        }
        return status(rtc_gpio_pulldown_dis(pin));
    }

    [[nodiscard]] static Status drive(const gpio_num_t pin, const gpio_drive_cap_t strength) noexcept
    {
        return status(rtc_gpio_set_drive_capability(pin, strength));
    }

    [[nodiscard]] static Result<gpio_drive_cap_t> drive(const gpio_num_t pin) noexcept
    {
        gpio_drive_cap_t out{};
        if (const auto err = rtc_gpio_get_drive_capability(pin, &out); err != ESP_OK) {
            return fail(err);
        }
        return ok(out);
    }

    [[nodiscard]] static Status hold(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_hold_en(pin));
    }

    [[nodiscard]] static Status unhold(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_hold_dis(pin));
    }

    [[nodiscard]] static Status hold_all() noexcept
    {
        return status(rtc_gpio_force_hold_en_all());
    }

    [[nodiscard]] static Status unhold_all() noexcept
    {
        return status(rtc_gpio_force_hold_dis_all());
    }

    [[nodiscard]] static Status isolate(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_isolate(pin));
    }

    [[nodiscard]] static Status wake(const gpio_num_t pin, const gpio_int_type_t edge) noexcept
    {
        return status(rtc_gpio_wakeup_enable(pin, edge));
    }

    [[nodiscard]] static Status wake_off(const gpio_num_t pin) noexcept
    {
        return status(rtc_gpio_wakeup_disable(pin));
    }
};

template <int Pin>
struct RtcPin {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid RTC GPIO pin number");

    using Resource = ClaimFor<ClaimKind::rtc_gpio, Pin, Pin>;

    [[nodiscard]] static constexpr gpio_num_t io() noexcept
    {
        return static_cast<gpio_num_t>(Pin);
    }

    [[nodiscard]] static bool valid() noexcept
    {
        return RtcGpio::valid(io());
    }

    [[nodiscard]] static Result<int> index() noexcept
    {
        return RtcGpio::index(io());
    }

    [[nodiscard]] static Status init() noexcept
    {
        if (const auto err = Resource::take(); err != ESP_OK) {
            return fail(err);
        }

        auto out = RtcGpio::init(io());
        if (!out) {
            Resource::drop();
        }
        return out;
    }

    [[nodiscard]] static Status deinit() noexcept
    {
        auto out = RtcGpio::deinit(io());
        if (out) {
            Resource::drop();
        }
        return out;
    }

    [[nodiscard]] static Status mode(const rtc_gpio_mode_t mode) noexcept
    {
        return RtcGpio::mode(io(), mode);
    }

    [[nodiscard]] static Status sleep_mode(const rtc_gpio_mode_t mode) noexcept
    {
        return RtcGpio::sleep_mode(io(), mode);
    }

    [[nodiscard]] static Result<bool> level() noexcept
    {
        return RtcGpio::level(io());
    }

    [[nodiscard]] static Status write(const bool high) noexcept
    {
        return RtcGpio::write(io(), high);
    }

    [[nodiscard]] static Status hi() noexcept
    {
        return write(true);
    }

    [[nodiscard]] static Status lo() noexcept
    {
        return write(false);
    }

    [[nodiscard]] static Status pullup(const bool enable = true) noexcept
    {
        return RtcGpio::pullup(io(), enable);
    }

    [[nodiscard]] static Status pulldown(const bool enable = true) noexcept
    {
        return RtcGpio::pulldown(io(), enable);
    }

    [[nodiscard]] static Status floating() noexcept
    {
        return RtcGpio::floating(io());
    }

    [[nodiscard]] static Status drive(const gpio_drive_cap_t strength) noexcept
    {
        return RtcGpio::drive(io(), strength);
    }

    [[nodiscard]] static Result<gpio_drive_cap_t> drive() noexcept
    {
        return RtcGpio::drive(io());
    }

    [[nodiscard]] static Status hold() noexcept
    {
        return RtcGpio::hold(io());
    }

    [[nodiscard]] static Status unhold() noexcept
    {
        return RtcGpio::unhold(io());
    }

    [[nodiscard]] static Status isolate() noexcept
    {
        return RtcGpio::isolate(io());
    }

    [[nodiscard]] static Status wake(const gpio_int_type_t edge) noexcept
    {
        return RtcGpio::wake(io(), edge);
    }

    [[nodiscard]] static Status wake_off() noexcept
    {
        return RtcGpio::wake_off(io());
    }

    [[nodiscard]] static bool held() noexcept
    {
        return Resource::held();
    }
};

}  // namespace arc
