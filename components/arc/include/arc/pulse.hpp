#pragma once

#include <cstdint>

#include "driver/mcpwm_prelude.h"
#include "esp_check.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"

namespace arc {

template <int Pin,
          std::uint32_t Hz,
          std::uint32_t DutyPermille = 500,
          int Group = 0,
          std::uint32_t ResolutionHz = 10'000'000,
          mcpwm_timer_clock_source_t Source = MCPWM_TIMER_CLK_SRC_DEFAULT,
          mcpwm_timer_count_mode_t Mode = MCPWM_TIMER_COUNT_MODE_UP>
struct Pulse {
    static_assert(Pin >= 0 && Pin < SOC_GPIO_PIN_COUNT, "invalid MCPWM pin");
    static_assert(Group >= 0, "invalid MCPWM group");
    static_assert(Hz != 0U, "MCPWM frequency must be non-zero");
    static_assert(ResolutionHz >= Hz, "MCPWM resolution must be at least the output frequency");
    static_assert(DutyPermille <= 1000U, "duty permille must be in [0, 1000]");
    static_assert(Mode == MCPWM_TIMER_COUNT_MODE_UP, "arc::Pulse currently supports up-count mode only");

    static void start()
    {
        init();
        set_hz(Hz);
        set_duty(DutyPermille);
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

    static void on() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.gen, 1, true));
    }

    static void off() noexcept
    {
        init();
        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.gen, 0, true));
    }

    static void wave() noexcept
    {
        init();
        apply();
    }

    static void hz(const std::uint32_t value) noexcept
    {
        init();
        set_hz(value);
        apply();
    }

    static void duty(const std::uint32_t permille) noexcept
    {
        init();
        set_duty(permille);
        apply();
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

private:
    struct State {
        mcpwm_timer_handle_t timer{};
        mcpwm_oper_handle_t oper{};
        mcpwm_cmpr_handle_t cmpr{};
        mcpwm_gen_handle_t gen{};
        std::uint32_t period{};
        std::uint32_t compare{};
        std::uint32_t duty{};
        bool enabled{};
        bool running{};
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

    static void set_hz(const std::uint32_t hz) noexcept
    {
        const auto period = period_ticks(hz);
        ESP_ERROR_CHECK(period > 0U ? ESP_OK : ESP_ERR_INVALID_ARG);
        state.period = period;
        ESP_ERROR_CHECK(mcpwm_timer_set_period(state.timer, state.period));
        state.compare = compare_ticks(state.period, state.duty);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(state.period, state.duty, state.compare)));
    }

    static void set_duty(const std::uint32_t permille) noexcept
    {
        ESP_ERROR_CHECK(permille <= 1000U ? ESP_OK : ESP_ERR_INVALID_ARG);
        state.duty = permille;
        state.compare = compare_ticks(state.period, state.duty);
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(state.period, state.duty, state.compare)));
    }

    static void apply() noexcept
    {
        if (state.duty == 0U) {
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.gen, 0, true));
            return;
        }

        if (state.duty >= 1000U) {
            ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.gen, 1, true));
            return;
        }

        ESP_ERROR_CHECK(mcpwm_generator_set_force_level(state.gen, -1, false));
    }

    static void run()
    {
        if (!state.enabled) {
            ESP_ERROR_CHECK(mcpwm_timer_enable(state.timer));
            state.enabled = true;
        }

        apply();

        if (!state.running) {
            ESP_ERROR_CHECK(mcpwm_timer_start_stop(state.timer, MCPWM_TIMER_START_NO_STOP));
            state.running = true;
        }
    }

    static void init()
    {
        if (state.timer != nullptr) {
            return;
        }

        state.duty = DutyPermille;
        state.period = period_ticks(Hz);
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
        oper.flags.update_dead_time_on_tez = 0U;
        oper.flags.update_dead_time_on_tep = 0U;
        oper.flags.update_dead_time_on_sync = 0U;
        ESP_ERROR_CHECK(mcpwm_new_operator(&oper, &state.oper));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(state.oper, state.timer));

        mcpwm_comparator_config_t cmp{};
        cmp.intr_priority = 0;
        cmp.flags.update_cmp_on_tez = 1U;
        cmp.flags.update_cmp_on_tep = 0U;
        cmp.flags.update_cmp_on_sync = 0U;
        ESP_ERROR_CHECK(mcpwm_new_comparator(state.oper, &cmp, &state.cmpr));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(
            state.cmpr,
            program_compare(state.period, state.duty, state.compare)));

        mcpwm_generator_config_t gen{};
        gen.gen_gpio_num = Pin;
        gen.flags.invert_pwm = 0U;
        ESP_ERROR_CHECK(mcpwm_new_generator(state.oper, &gen, &state.gen));

        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
            state.gen,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
        ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
            state.gen,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, state.cmpr, MCPWM_GEN_ACTION_LOW)));
    }
};

}  // namespace arc
