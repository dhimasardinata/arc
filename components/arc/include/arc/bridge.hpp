#pragma once

#include <cstdint>

#include "driver/mcpwm_prelude.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

#include "arc/init.hpp"

namespace arc {

template <int HighPin,
          int LowPin,
          std::uint32_t Hz,
          std::uint32_t DutyPermille = 500,
          std::uint32_t RiseDelayTicks = 0,
          std::uint32_t FallDelayTicks = 0,
          int Group = 0,
          std::uint32_t ResolutionHz = 10'000'000,
          mcpwm_timer_clock_source_t Source = MCPWM_TIMER_CLK_SRC_DEFAULT,
          mcpwm_timer_count_mode_t Mode = MCPWM_TIMER_COUNT_MODE_UP,
          int FaultPin = -1,
          bool FaultActiveHigh = true,
          mcpwm_operator_brake_mode_t BrakeMode = MCPWM_OPER_BRAKE_MODE_CBC,
          mcpwm_generator_action_t BrakeHighAction = MCPWM_GEN_ACTION_LOW,
          mcpwm_generator_action_t BrakeLowAction = MCPWM_GEN_ACTION_LOW,
          bool RecoverOnEmpty = true,
          bool RecoverOnPeak = false>
struct Bridge {
    static_assert(HighPin >= 0 && HighPin < SOC_GPIO_PIN_COUNT, "invalid MCPWM high pin");
    static_assert(LowPin >= 0 && LowPin < SOC_GPIO_PIN_COUNT, "invalid MCPWM low pin");
    static_assert((FaultPin >= 0 && FaultPin < SOC_GPIO_PIN_COUNT) || FaultPin == -1, "invalid MCPWM fault pin");
    static_assert(HighPin != LowPin, "bridge outputs must use distinct pins");
    static_assert(FaultPin == -1 || (FaultPin != HighPin && FaultPin != LowPin), "bridge fault pin must be distinct");
    static_assert(Group >= 0, "invalid MCPWM group");
    static_assert(Hz != 0U, "MCPWM frequency must be non-zero");
    static_assert(ResolutionHz >= Hz, "MCPWM resolution must be at least the output frequency");
    static_assert(DutyPermille <= 1000U, "duty permille must be in [0, 1000]");
    static_assert(Mode == MCPWM_TIMER_COUNT_MODE_UP, "arc::Bridge currently supports up-count mode only");
    static_assert(
        BrakeMode == MCPWM_OPER_BRAKE_MODE_CBC || BrakeMode == MCPWM_OPER_BRAKE_MODE_OST,
        "invalid MCPWM brake mode");

    struct Config {
        std::uint32_t hz{Hz};
        std::uint32_t duty{DutyPermille};
    };

    [[nodiscard]] static constexpr std::uint32_t frequency() noexcept
    {
        return Hz;
    }

    [[nodiscard]] static constexpr std::uint32_t duty_permille() noexcept
    {
        return DutyPermille;
    }

    [[nodiscard]] static constexpr std::uint32_t rise_ticks() noexcept
    {
        return RiseDelayTicks;
    }

    [[nodiscard]] static constexpr std::uint32_t fall_ticks() noexcept
    {
        return FallDelayTicks;
    }

    [[nodiscard]] static constexpr Config defaults() noexcept
    {
        return Config{};
    }

    [[nodiscard]] static std::uint32_t hz() noexcept
    {
        init();
        return state.hz;
    }

    [[nodiscard]] static Config config() noexcept
    {
        init();
        return Config{
            .hz = state.hz,
            .duty = state.duty,
        };
    }

    static void start()
    {
        start(defaults());
    }

    static void start(const Config& cfg)
    {
        init();
        ESP_ERROR_CHECK(set(cfg));
        run();
    }

    static void stop()
    {
        if (!state.running) {
            return;
        }

        ESP_ERROR_CHECK(mcpwm_timer_start_stop(state.timer, MCPWM_TIMER_STOP_EMPTY));
        state.running = false;
    }

    static void pause()
    {
        stop();
    }

    static void resume()
    {
        run();
    }

    static void off() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.high, 0, true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.low, 0, true));
    }

    static void hi() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.high, 1, true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.low, 0, true));
    }

    static void lo() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.high, 0, true));
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.low, 1, true));
    }

    static void wave() noexcept
    {
        init();
        ESP_ERROR_CHECK(program_wave());
    }

    static void hz(const std::uint32_t value) noexcept
    {
        init();
        ESP_ERROR_CHECK(set(Config{
            .hz = value,
            .duty = permille(),
        }));
    }

    static void duty(const std::uint32_t permille) noexcept
    {
        init();
        ESP_ERROR_CHECK(set(permille));
    }

    [[nodiscard]] static constexpr std::uint32_t resolution() noexcept
    {
        return ResolutionHz;
    }

    [[nodiscard]] static std::uint32_t period() noexcept
    {
        init();
        return state.period;
    }

    [[nodiscard]] static std::uint32_t compare() noexcept
    {
        init();
        return state.compare;
    }

    [[nodiscard]] static std::uint32_t permille() noexcept
    {
        init();
        return state.duty;
    }

    [[nodiscard]] static esp_err_t set(const Config& cfg) noexcept
    {
        if (cfg.hz == 0U || cfg.duty > 1000U) {
            return ESP_ERR_INVALID_ARG;
        }

        init();

        const auto err = program_hz(cfg.hz);
        if (err != ESP_OK) {
            return err;
        }

        const auto duty_err = program_duty(cfg.duty);
        if (duty_err != ESP_OK) {
            return duty_err;
        }

        return program_wave();
    }

    [[nodiscard]] static esp_err_t set(const std::uint32_t permille) noexcept
    {
        init();

        const auto err = program_duty(permille);
        if (err != ESP_OK) {
            return err;
        }

        return program_wave();
    }

    [[nodiscard]] static esp_err_t recover() noexcept
    {
        if constexpr (FaultPin < 0) {
            return ESP_ERR_INVALID_STATE;
        } else {
            init();
            return mcpwm_operator_recover_from_fault(state.oper, state.fault);
        }
    }

private:
    struct State {
        mcpwm_timer_handle_t timer{};
        mcpwm_oper_handle_t oper{};
        mcpwm_cmpr_handle_t cmpr{};
        mcpwm_gen_handle_t high{};
        mcpwm_gen_handle_t low{};
        mcpwm_fault_handle_t fault{};
        std::uint32_t hz{};
        std::uint32_t period{};
        std::uint32_t compare{};
        std::uint32_t duty{};
        bool enabled{};
        bool running{};
        std::uint32_t init{};
    };

    constinit static inline State state{};

    [[nodiscard]] static constexpr std::uint32_t period_ticks(const std::uint32_t hz) noexcept
    {
        return hz == 0U ? 0U : (ResolutionHz + (hz / 2U)) / hz;
    }

    [[nodiscard]] static constexpr std::uint32_t compare_ticks(
        const std::uint32_t period,
        const std::uint32_t duty) noexcept
    {
        return static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(period) * static_cast<std::uint64_t>(duty) + 500ULL) / 1000ULL);
    }

    [[nodiscard]] static constexpr std::uint32_t program_compare(
        const std::uint32_t period,
        const std::uint32_t duty,
        const std::uint32_t compare) noexcept
    {
        if (period == 0U) {
            return 0U;
        }

        if (duty >= 1000U) {
            return period > 1U ? period - 1U : 0U;
        }

        return compare;
    }

    [[nodiscard]] static esp_err_t program_hz(const std::uint32_t hz) noexcept
    {
        const auto period = period_ticks(hz);
        if (period == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto compare = compare_ticks(period, state.duty);
        auto err = mcpwm_timer_set_period(state.timer, period);
        if (err != ESP_OK) {
            return err;
        }

        err = mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(period, state.duty, compare));
        if (err != ESP_OK) {
            return err;
        }

        state.hz = hz;
        state.period = period;
        state.compare = compare;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t program_duty(const std::uint32_t permille) noexcept
    {
        if (permille > 1000U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto compare = compare_ticks(state.period, permille);
        const auto err = mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(state.period, permille, compare));
        if (err != ESP_OK) {
            return err;
        }

        state.duty = permille;
        state.compare = compare;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t program_wave() noexcept
    {
        if (state.duty == 0U) {
            auto err = mcpwm_generator_set_force_level(state.high, 0, true);
            if (err != ESP_OK) {
                return err;
            }
            return mcpwm_generator_set_force_level(state.low, 0, true);
        }

        if (state.duty >= 1000U) {
            auto err = mcpwm_generator_set_force_level(state.high, 1, true);
            if (err != ESP_OK) {
                return err;
            }
            return mcpwm_generator_set_force_level(state.low, 0, true);
        }

        auto err = mcpwm_generator_set_force_level(state.high, -1, false);
        if (err != ESP_OK) {
            return err;
        }
        return mcpwm_generator_set_force_level(state.low, -1, false);
    }

    static void run()
    {
        if (!state.enabled) {
            ESP_ERROR_CHECK(mcpwm_timer_enable(state.timer));
            state.enabled = true;
        }

        ESP_ERROR_CHECK(program_wave());

        if (!state.running) {
            ESP_ERROR_CHECK(mcpwm_timer_start_stop(state.timer, MCPWM_TIMER_START_NO_STOP));
            state.running = true;
        }
    }

    static void init()
    {
        if (!Init::begin(state.init)) {
            return;
        }

        state.hz = Hz;
        state.duty = DutyPermille;
        state.period = period_ticks(state.hz);
        state.compare = compare_ticks(state.period, state.duty);

        mcpwm_timer_config_t timer{};
        timer.group_id = Group;
        timer.clk_src = Source;
        timer.resolution_hz = ResolutionHz;
        timer.count_mode = Mode;
        timer.period_ticks = state.period;
        timer.intr_priority = 0;
        timer.flags.update_period_on_empty = 1U;
        timer.flags.update_period_on_sync = 0U;
        timer.flags.allow_pd = 0U;
        ESP_ERROR_CHECK(mcpwm_new_timer(&timer, &state.timer));

        mcpwm_operator_config_t oper{};
        oper.group_id = Group;
        oper.intr_priority = 0;
        oper.flags.update_gen_action_on_tez = 1U;
        oper.flags.update_gen_action_on_tep = 0U;
        oper.flags.update_gen_action_on_sync = 0U;
        oper.flags.update_dead_time_on_tez = 1U;
        oper.flags.update_dead_time_on_tep = 0U;
        oper.flags.update_dead_time_on_sync = 0U;
        ESP_ERROR_CHECK(mcpwm_new_operator(&oper, &state.oper));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(state.oper, state.timer));

        if constexpr (FaultPin >= 0) {
            mcpwm_gpio_fault_config_t fault{};
            fault.group_id = Group;
            fault.intr_priority = 0;
            fault.gpio_num = FaultPin;
            fault.flags.active_level = FaultActiveHigh ? 1U : 0U;
            ESP_ERROR_CHECK(mcpwm_new_gpio_fault(&fault, &state.fault));

            mcpwm_brake_config_t brake{};
            brake.fault = state.fault;
            brake.brake_mode = BrakeMode;
            brake.flags.cbc_recover_on_tez = RecoverOnEmpty ? 1U : 0U;
            brake.flags.cbc_recover_on_tep = RecoverOnPeak ? 1U : 0U;
            ESP_ERROR_CHECK(mcpwm_operator_set_brake_on_fault(state.oper, &brake));
        }

        mcpwm_comparator_config_t cmp{};
        cmp.intr_priority = 0;
        cmp.flags.update_cmp_on_tez = 1U;
        cmp.flags.update_cmp_on_tep = 0U;
        cmp.flags.update_cmp_on_sync = 0U;
        ESP_ERROR_CHECK(mcpwm_new_comparator(state.oper, &cmp, &state.cmpr));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(state.period, state.duty, state.compare)));

        mcpwm_generator_config_t high{};
        high.gen_gpio_num = HighPin;
        high.flags.invert_pwm = 0U;
        ESP_ERROR_CHECK(mcpwm_new_generator(state.oper, &high, &state.high));

        mcpwm_generator_config_t low{};
        low.gen_gpio_num = LowPin;
        low.flags.invert_pwm = 0U;
        ESP_ERROR_CHECK(mcpwm_new_generator(state.oper, &low, &state.low));

        if constexpr (FaultPin >= 0) {
            ESP_ERROR_CHECK(mcpwm_generator_set_action_on_brake_event(
                state.high,
                MCPWM_GEN_BRAKE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, BrakeMode, BrakeHighAction)));
            ESP_ERROR_CHECK(mcpwm_generator_set_action_on_brake_event(
                state.low,
                MCPWM_GEN_BRAKE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, BrakeMode, BrakeLowAction)));
        }

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
            state.high,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
            state.high,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, state.cmpr, MCPWM_GEN_ACTION_LOW)));

        mcpwm_dead_time_config_t high_dead{};
        high_dead.posedge_delay_ticks = RiseDelayTicks;
        high_dead.negedge_delay_ticks = 0U;
        high_dead.flags.invert_output = 0U;
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(state.high, state.high, &high_dead));

        mcpwm_dead_time_config_t low_dead{};
        low_dead.posedge_delay_ticks = 0U;
        low_dead.negedge_delay_ticks = FallDelayTicks;
        low_dead.flags.invert_output = 1U;
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(state.high, state.low, &low_dead));
        Init::pass(state.init);
    }
};

}  // namespace arc
