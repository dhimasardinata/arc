#pragma once

#include <cstdint>
#include <limits>

namespace arc {

struct TimeSyncSample {
    std::int64_t local_tx{};
    std::int64_t remote_rx{};
    std::int64_t remote_tx{};
    std::int64_t local_rx{};
};

struct TimeSyncHwSample {
    std::int64_t local_tx{};
    std::int64_t remote_rx{};
    std::int64_t remote_tx{};
    std::int64_t local_rx{};
    std::int32_t local_shift{};
    std::int32_t remote_shift{};
};

struct TimeSyncConfig {
    std::int32_t kp_shift{3};
    std::int32_t ki_shift{8};
    std::int64_t step_max{250'000};
    std::int64_t integral_max{1'000'000};
};

struct TimeSyncStats {
    std::int64_t raw_offset{};
    std::int64_t filt_offset{};
    std::int64_t delay{};
    std::int64_t correction{};
    std::uint32_t samples{};
};

struct PtpSample {
    std::int64_t origin_ns{};
    std::int64_t ingress_ns{};
    std::int64_t egress_ns{};
    std::int64_t receive_ns{};
};

struct PtpHwSample {
    std::int64_t origin{};
    std::int64_t ingress{};
    std::int64_t egress{};
    std::int64_t receive{};
    std::int32_t origin_shift{};
    std::int32_t ingress_shift{};
    std::int32_t egress_shift{};
    std::int32_t receive_shift{};
};

struct PtpConfig {
    std::int32_t kp_shift{4};
    std::int32_t ki_shift{10};
    std::int64_t step_max{1'000'000};
    std::int64_t integral_max{10'000'000};
};

struct PtpStats {
    std::int64_t raw_offset{};
    std::int64_t filt_offset{};
    std::int64_t delay{};
    std::int64_t correction{};
    std::uint32_t samples{};
};

struct TimeSync {
    std::int64_t offset_us{};
    std::int64_t integral_us{};
    TimeSyncStats stats{};

    void clear() noexcept
    {
        *this = {};
    }

    [[nodiscard]] std::int64_t to_remote(const std::int64_t local_us) const noexcept
    {
        return sat_add(local_us, offset_us);
    }

    [[nodiscard]] std::int64_t to_local(const std::int64_t remote_us) const noexcept
    {
        return sat_sub(remote_us, offset_us);
    }

    [[nodiscard]] TimeSyncStats discipline(
        const TimeSyncSample sample,
        const TimeSyncConfig config = {}) noexcept
    {
        const auto leg_out = sat_sub(sample.remote_rx, sample.local_tx);
        const auto leg_back = sat_sub(sample.remote_tx, sample.local_rx);
        const auto measured = div2(sat_add(leg_out, leg_back));
        const auto remote_turn = sat_sub(sample.remote_tx, sample.remote_rx);
        const auto local_span = sat_sub(sample.local_rx, sample.local_tx);
        const auto delay = sat_sub(local_span, remote_turn);
        const auto error = sat_sub(measured, offset_us);

        integral_us = clamp(
            sat_add(integral_us, error),
            -config.integral_max,
            config.integral_max);

        const auto p = shift(error, config.kp_shift);
        const auto i = shift(integral_us, config.ki_shift);
        const auto correction = clamp(sat_add(p, i), -config.step_max, config.step_max);
        offset_us = sat_add(offset_us, correction);

        stats = TimeSyncStats{
            .raw_offset = measured,
            .filt_offset = offset_us,
            .delay = delay,
            .correction = correction,
            .samples = stats.samples + 1U,
        };
        return stats;
    }

    [[nodiscard]] TimeSyncStats discipline_hw(
        const TimeSyncHwSample sample,
        const TimeSyncConfig config = {}) noexcept
    {
        return discipline(
            {
                .local_tx = shift(sample.local_tx, sample.local_shift),
                .remote_rx = shift(sample.remote_rx, sample.remote_shift),
                .remote_tx = shift(sample.remote_tx, sample.remote_shift),
                .local_rx = shift(sample.local_rx, sample.local_shift),
            },
            config);
    }

    [[nodiscard]] static constexpr std::int64_t clamp(
        const std::int64_t value,
        const std::int64_t lo,
        const std::int64_t hi) noexcept
    {
        return value < lo ? lo : (value > hi ? hi : value);
    }

    [[nodiscard]] static constexpr std::int64_t div2(const std::int64_t value) noexcept
    {
        return value / 2;
    }

    [[nodiscard]] static constexpr std::int64_t shift(
        const std::int64_t value,
        const std::int32_t bits) noexcept
    {
        if (bits <= 0) {
            return value;
        }
        if (bits >= 62) {
            return 0;
        }
        return value / (std::int64_t{1} << bits);
    }

    [[nodiscard]] static constexpr std::int64_t sat_add(
        const std::int64_t lhs,
        const std::int64_t rhs) noexcept
    {
        if (rhs > 0 && lhs > (std::numeric_limits<std::int64_t>::max() - rhs)) {
            return std::numeric_limits<std::int64_t>::max();
        }
        if (rhs < 0 && lhs < (std::numeric_limits<std::int64_t>::min() - rhs)) {
            return std::numeric_limits<std::int64_t>::min();
        }
        return lhs + rhs;
    }

    [[nodiscard]] static constexpr std::int64_t sat_sub(
        const std::int64_t lhs,
        const std::int64_t rhs) noexcept
    {
        if (rhs == std::numeric_limits<std::int64_t>::min()) {
            return std::numeric_limits<std::int64_t>::max();
        }
        return sat_add(lhs, -rhs);
    }
};

struct PtpClock {
    std::int64_t offset_ns{};
    std::int64_t integral_ns{};
    PtpStats stats{};

    void clear() noexcept
    {
        *this = {};
    }

    [[nodiscard]] std::int64_t to_master(const std::int64_t local_ns) const noexcept
    {
        return TimeSync::sat_add(local_ns, offset_ns);
    }

    [[nodiscard]] std::int64_t to_local(const std::int64_t remote_ns) const noexcept
    {
        return TimeSync::sat_sub(remote_ns, offset_ns);
    }

    [[nodiscard]] PtpStats discipline(
        const PtpSample sample,
        const PtpConfig config = {}) noexcept
    {
        const auto leg_out = TimeSync::sat_sub(sample.ingress_ns, sample.origin_ns);
        const auto leg_back = TimeSync::sat_sub(sample.egress_ns, sample.receive_ns);
        const auto measured = TimeSync::div2(TimeSync::sat_add(leg_out, leg_back));
        const auto remote_turn = TimeSync::sat_sub(sample.egress_ns, sample.ingress_ns);
        const auto local_span = TimeSync::sat_sub(sample.receive_ns, sample.origin_ns);
        const auto delay = TimeSync::sat_sub(local_span, remote_turn);
        const auto error = TimeSync::sat_sub(measured, offset_ns);

        integral_ns = TimeSync::clamp(
            TimeSync::sat_add(integral_ns, error),
            -config.integral_max,
            config.integral_max);

        const auto p = TimeSync::shift(error, config.kp_shift);
        const auto i = TimeSync::shift(integral_ns, config.ki_shift);
        const auto correction = TimeSync::clamp(
            TimeSync::sat_add(p, i),
            -config.step_max,
            config.step_max);
        offset_ns = TimeSync::sat_add(offset_ns, correction);

        stats = PtpStats{
            .raw_offset = measured,
            .filt_offset = offset_ns,
            .delay = delay,
            .correction = correction,
            .samples = stats.samples + 1U,
        };
        return stats;
    }

    [[nodiscard]] PtpStats discipline_hw(
        const PtpHwSample sample,
        const PtpConfig config = {}) noexcept
    {
        return discipline(
            {
                .origin_ns = TimeSync::shift(sample.origin, sample.origin_shift),
                .ingress_ns = TimeSync::shift(sample.ingress, sample.ingress_shift),
                .egress_ns = TimeSync::shift(sample.egress, sample.egress_shift),
                .receive_ns = TimeSync::shift(sample.receive, sample.receive_shift),
            },
            config);
    }
};

}  // namespace arc
