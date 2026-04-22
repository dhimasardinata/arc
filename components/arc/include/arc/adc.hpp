#pragma once

#include <cstdint>

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/claim.hpp"
#include "arc/init.hpp"
#include "arc/result.hpp"

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

    [[nodiscard]] static constexpr adc_bitwidth_t bitwidth() noexcept
    {
        return static_cast<adc_bitwidth_t>(Width);
    }

    static void resolve(adc_unit_t& unit, adc_channel_t& channel) noexcept
    {
        ESP_ERROR_CHECK(adc_continuous_io_to_channel(Io, &unit, &channel));
        ESP_ERROR_CHECK(unit == ADC_UNIT_1 ? ESP_OK : ESP_ERR_NOT_SUPPORTED);
    }

    [[nodiscard]] static esp_err_t resolve_one(adc_unit_t& unit, adc_channel_t& channel) noexcept
    {
        return adc_oneshot_io_to_channel(Io, &unit, &channel);
    }
};

template <adc_unit_t Unit = ADC_UNIT_1,
          adc_oneshot_clk_src_t Clock = ADC_RTC_CLK_SRC_DEFAULT,
          adc_ulp_mode_t Ulp = ADC_ULP_MODE_DISABLE>
struct AdcBus {
    static_assert(SOC_ADC_SUPPORTED, "ADC is not supported on this target");

    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        auto err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        adc_oneshot_unit_init_cfg_t cfg{};
        cfg.unit_id = Unit;
        cfg.clk_src = Clock;
        cfg.ulp_mode = Ulp;
        err = adc_oneshot_new_unit(&cfg, &state.handle);
        if (err == ESP_OK) {
            Init::pass(state.init);
        } else {
            state.handle = nullptr;
            Resource::drop();
            Init::fail(state.init);
        }
        return err;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static adc_oneshot_unit_handle_t native() noexcept
    {
        boot();
        return state.handle;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        const auto err = adc_oneshot_del_unit(state.handle);
        if (err != ESP_OK) {
            Init::pass(state.init);
            return err;
        }

        state.handle = nullptr;
        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static constexpr adc_unit_t unit() noexcept
    {
        return Unit;
    }

private:
    using Resource = Claim<ClaimKind::adc_bus,
                           static_cast<int>(Unit),
                           claim_token<Unit, Clock, Ulp>()>;

    struct State {
        adc_oneshot_unit_handle_t handle;
        std::uint32_t init;
    };

    constinit static inline State state{nullptr, 0U};
};

template <typename Bus,
          typename Pad,
          bool Cali = true>
struct AdcOne {
    [[nodiscard]] static esp_err_t init() noexcept
    {
        if (!Init::begin(state.init)) {
            return ESP_OK;
        }

        adc_unit_t unit{};
        adc_channel_t channel{};
        auto err = Pad::resolve_one(unit, channel);
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }
        if (unit != Bus::unit()) {
            Init::fail(state.init);
            return ESP_ERR_INVALID_ARG;
        }

        err = Bus::init();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        err = Resource::take();
        if (err != ESP_OK) {
            Init::fail(state.init);
            return err;
        }

        adc_oneshot_chan_cfg_t cfg{};
        cfg.atten = Pad::atten();
        cfg.bitwidth = Pad::bitwidth();
        err = adc_oneshot_config_channel(Bus::native(), channel, &cfg);
        if (err != ESP_OK) {
            Resource::drop();
            Init::fail(state.init);
            return err;
        }

        state.channel = channel;
        Init::pass(state.init);
        return ESP_OK;
    }

    static void boot()
    {
        ESP_ERROR_CHECK(init());
    }

    [[nodiscard]] static esp_err_t raw(int& out) noexcept
    {
        const auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        return adc_oneshot_read(Bus::native(), state.channel, &out);
    }

    [[nodiscard]] static Result<int> raw() noexcept
    {
        int out{};
        const auto err = raw(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t mv(int& out) noexcept
        requires(Cali)
    {
        auto err = init();
        if (err != ESP_OK) {
            return err;
        }
        err = cali();
        if (err != ESP_OK) {
            return err;
        }
        return adc_oneshot_get_calibrated_result(Bus::native(), state.cali, state.channel, &out);
    }

    [[nodiscard]] static Result<int> mv() noexcept
        requires(Cali)
    {
        int out{};
        const auto err = mv(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        if constexpr (Cali) {
            const auto err = cali_off();
            if (err != ESP_OK) {
                return err;
            }
        }

        if (!Init::take(state.init)) {
            return ESP_OK;
        }

        Resource::drop();
        Init::fail(state.init);
        return ESP_OK;
    }

    [[nodiscard]] static constexpr int io() noexcept
    {
        return Pad::io();
    }

private:
    using Resource = Claim<ClaimKind::adc_dev,
                           (static_cast<int>(Bus::unit()) * SOC_GPIO_PIN_COUNT) + Pad::io(),
                           claim_token<Bus::unit(), Pad::io(), Pad::atten(), Pad::width(), Cali>()>;

    struct State {
        adc_channel_t channel{};
        adc_cali_handle_t cali{};
        std::uint32_t init{};
        std::uint32_t cali_init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static esp_err_t cali() noexcept
        requires(Cali)
    {
        if (!Init::begin(state.cali_init)) {
            return ESP_OK;
        }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cfg{};
        cfg.unit_id = Bus::unit();
        cfg.chan = state.channel;
        cfg.atten = Pad::atten();
        cfg.bitwidth = Pad::bitwidth();
        const auto err = adc_cali_create_scheme_curve_fitting(&cfg, &state.cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cfg{};
        cfg.unit_id = Bus::unit();
        cfg.atten = Pad::atten();
        cfg.bitwidth = Pad::bitwidth();
        const auto err = adc_cali_create_scheme_line_fitting(&cfg, &state.cali);
#else
        const auto err = ESP_ERR_NOT_SUPPORTED;
#endif

        if (err == ESP_OK) {
            Init::pass(state.cali_init);
        } else {
            state.cali = nullptr;
            Init::fail(state.cali_init);
        }
        return err;
    }

    [[nodiscard]] static esp_err_t cali_off() noexcept
        requires(Cali)
    {
        if (!Init::take(state.cali_init)) {
            return ESP_OK;
        }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        const auto err = adc_cali_delete_scheme_curve_fitting(state.cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        const auto err = adc_cali_delete_scheme_line_fitting(state.cali);
#else
        const auto err = ESP_OK;
#endif

        if (err == ESP_OK) {
            state.cali = nullptr;
            Init::fail(state.cali_init);
        } else {
            Init::pass(state.cali_init);
        }
        return err;
    }
};

}  // namespace arc
