#pragma once

#include <cstdint>

#include "esp_adc/adc_continuous.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Io,
          adc_atten_t Atten = ADC_ATTEN_DB_12,
          std::uint32_t Width = SOC_ADC_DIGI_MAX_BITWIDTH>
struct Adc {
    static_assert(Io >= 0 && Io < SOC_GPIO_PIN_COUNT, "invalid ADC GPIO");
    static_assert(Width > 0U && Width <= SOC_ADC_DIGI_MAX_BITWIDTH, "invalid ADC bit width");

    [[nodiscard]] static constexpr int io() noexcept
    {
        return Io;
    }

    [[nodiscard]] static constexpr adc_atten_t atten() noexcept
    {
        return Atten;
    }

    [[nodiscard]] static constexpr std::uint32_t width() noexcept
    {
        return Width;
    }

    static void resolve(adc_unit_t& unit, adc_channel_t& channel) noexcept
    {
        ESP_ERROR_CHECK(adc_continuous_io_to_channel(Io, &unit, &channel));
        ESP_ERROR_CHECK(unit == ADC_UNIT_1 ? ESP_OK : ESP_ERR_NOT_SUPPORTED);
    }
};

}  // namespace arc
