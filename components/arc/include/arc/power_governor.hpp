#pragma once

#include <cstdint>
#include <limits>

#include "arc/kalman.hpp"
#include "arc/probe.hpp"
#include "arc/result.hpp"

namespace arc::power {

enum class GovernorAction : std::uint8_t {
    hold,
    boost,
    release,
};

struct GovernorConfig {
    std::int32_t guard_cycles{0};
    std::uint32_t min_samples{2U};
    float process_noise{1.0F};
    float measurement_noise{4.0F};
};

struct GovernorDecision {
    GovernorAction action{GovernorAction::hold};
    std::int32_t observed_slack{};
    std::int32_t predicted_slack{};
    std::uint32_t samples{};
    bool overrun_risk{};
};

template <typename T = float>
struct Governor {
    DeadlineStats deadlines{};
    dsp::Kalman<T, 1, 1> slack_filter{};
    bool seeded{};

    [[nodiscard]] GovernorDecision decide(
        const esp_cpu_cycle_count_t elapsed,
        const esp_cpu_cycle_count_t budget,
        const GovernorConfig config = {}) noexcept
    {
        deadlines.add(elapsed, budget);
        const auto observed = slack(elapsed, budget);
        if (!seeded) {
            slack_filter.x(0, 0) = static_cast<T>(observed);
            seeded = true;
        } else {
            typename dsp::Kalman<T, 1, 1>::A a{};
            typename dsp::Kalman<T, 1, 1>::Q q{};
            typename dsp::Kalman<T, 1, 1>::H h{};
            typename dsp::Kalman<T, 1, 1>::R r{};
            typename dsp::Kalman<T, 1, 1>::Measure z{};
            a(0, 0) = T{1};
            q(0, 0) = static_cast<T>(config.process_noise);
            h(0, 0) = T{1};
            r(0, 0) = static_cast<T>(config.measurement_noise);
            z(0, 0) = static_cast<T>(observed);
            slack_filter.predict(a, q);
            slack_filter.correct_diagonal(h, r, z);
        }

        const auto predicted = static_cast<std::int32_t>(slack_filter.x(0, 0));
        const auto enough = deadlines.samples >= config.min_samples;
        const auto risk = predicted <= config.guard_cycles || deadlines.max_overrun > 0;
        const auto action = !enough ? GovernorAction::hold : risk ? GovernorAction::boost
                                                                  : GovernorAction::release;
        return {
            .action = action,
            .observed_slack = observed,
            .predicted_slack = predicted,
            .samples = deadlines.samples,
            .overrun_risk = risk,
        };
    }

    template <typename Policy>
    [[nodiscard]] static Status apply(const GovernorDecision decision) noexcept
    {
        switch (decision.action) {
            case GovernorAction::hold:
                return ok();
            case GovernorAction::boost:
                return status(Policy::cpu_boost());
            case GovernorAction::release:
                return status(Policy::cpu_release());
        }
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    template <typename Policy>
    [[nodiscard]] Result<GovernorDecision> tick(
        const esp_cpu_cycle_count_t elapsed,
        const esp_cpu_cycle_count_t budget,
        const GovernorConfig config = {}) noexcept
    {
        const auto decision = decide(elapsed, budget, config);
        const auto applied = apply<Policy>(decision);
        if (!applied) {
            return fail(applied.error());
        }
        return decision;
    }

private:
    [[nodiscard]] static constexpr std::int32_t slack(
        const esp_cpu_cycle_count_t elapsed,
        const esp_cpu_cycle_count_t budget) noexcept
    {
        const auto diff = static_cast<std::int64_t>(budget) - static_cast<std::int64_t>(elapsed);
        if (diff < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())) {
            return std::numeric_limits<std::int32_t>::min();
        }
        if (diff > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
            return std::numeric_limits<std::int32_t>::max();
        }
        return static_cast<std::int32_t>(diff);
    }
};

}  // namespace arc::power
