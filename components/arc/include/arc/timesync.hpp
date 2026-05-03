#pragma once

#include <cstdint>
#include <limits>

namespace arc {

struct TimeSyncSample {
    std::int64_t local_send_us{};
    std::int64_t remote_recv_us{};
    std::int64_t remote_send_us{};
    std::int64_t local_recv_us{};
};

struct TimeSyncHwSample {
    std::int64_t local_send_hw{};
    std::int64_t remote_recv_hw{};
    std::int64_t remote_send_hw{};
    std::int64_t local_recv_hw{};
    std::int32_t local_hw_to_us_shift{};
    std::int32_t remote_hw_to_us_shift{};
};

struct TimeSyncConfig {
    std::int32_t kp_shift{3};
    std::int32_t ki_shift{8};
    std::int64_t max_step_us{250'000};
    std::int64_t max_integral_us{1'000'000};
};

struct TimeSyncStats {
    std::int64_t raw_offset_us{};
    std::int64_t filtered_offset_us{};
    std::int64_t delay_us{};
    std::int64_t correction_us{};
    std::uint32_t samples{};
};

struct PtpSample {
    std::int64_t origin_ns{};
    std::int64_t ingress_ns{};
    std::int64_t egress_ns{};
    std::int64_t receive_ns{};
};

struct PtpConfig {
    std::int32_t kp_shift{4};
    std::int32_t ki_shift{10};
    std::int64_t max_step_ns{1'000'000};
    std::int64_t max_integral_ns{10'000'000};
};

struct PtpStats {
    std::int64_t raw_offset_ns{};
    std::int64_t filtered_offset_ns{};
    std::int64_t delay_ns{};
    std::int64_t correction_ns{};
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

    [[nodiscard]] std::int64_t local_to_remote(const std::int64_t local_us) const noexcept
    {
        return saturating_add(local_us, offset_us);
    }

    [[nodiscard]] std::int64_t remote_to_local(const std::int64_t remote_us) const noexcept
    {
        return saturating_sub(remote_us, offset_us);
    }

    [[nodiscard]] TimeSyncStats discipline(
        const TimeSyncSample sample,
        const TimeSyncConfig config = {}) noexcept
    {
        const auto leg_out = saturating_sub(sample.remote_recv_us, sample.local_send_us);
        const auto leg_back = saturating_sub(sample.remote_send_us, sample.local_recv_us);
        const auto measured = div2(saturating_add(leg_out, leg_back));
        const auto remote_turn = saturating_sub(sample.remote_send_us, sample.remote_recv_us);
        const auto local_span = saturating_sub(sample.local_recv_us, sample.local_send_us);
        const auto delay = saturating_sub(local_span, remote_turn);
        const auto error = saturating_sub(measured, offset_us);

        integral_us = clamp(
            saturating_add(integral_us, error),
            -config.max_integral_us,
            config.max_integral_us);

        const auto p = shift(error, config.kp_shift);
        const auto i = shift(integral_us, config.ki_shift);
        const auto correction = clamp(saturating_add(p, i), -config.max_step_us, config.max_step_us);
        offset_us = saturating_add(offset_us, correction);

        stats = TimeSyncStats{
            .raw_offset_us = measured,
            .filtered_offset_us = offset_us,
            .delay_us = delay,
            .correction_us = correction,
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
                .local_send_us = shift(sample.local_send_hw, sample.local_hw_to_us_shift),
                .remote_recv_us = shift(sample.remote_recv_hw, sample.remote_hw_to_us_shift),
                .remote_send_us = shift(sample.remote_send_hw, sample.remote_hw_to_us_shift),
                .local_recv_us = shift(sample.local_recv_hw, sample.local_hw_to_us_shift),
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

    [[nodiscard]] static constexpr std::int64_t saturating_add(
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

    [[nodiscard]] static constexpr std::int64_t saturating_sub(
        const std::int64_t lhs,
        const std::int64_t rhs) noexcept
    {
        if (rhs == std::numeric_limits<std::int64_t>::min()) {
            return std::numeric_limits<std::int64_t>::max();
        }
        return saturating_add(lhs, -rhs);
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

    [[nodiscard]] std::int64_t local_to_grandmaster(const std::int64_t local_ns) const noexcept
    {
        return TimeSync::saturating_add(local_ns, offset_ns);
    }

    [[nodiscard]] std::int64_t grandmaster_to_local(const std::int64_t remote_ns) const noexcept
    {
        return TimeSync::saturating_sub(remote_ns, offset_ns);
    }

    [[nodiscard]] PtpStats discipline(
        const PtpSample sample,
        const PtpConfig config = {}) noexcept
    {
        const auto leg_out = TimeSync::saturating_sub(sample.ingress_ns, sample.origin_ns);
        const auto leg_back = TimeSync::saturating_sub(sample.egress_ns, sample.receive_ns);
        const auto measured = TimeSync::div2(TimeSync::saturating_add(leg_out, leg_back));
        const auto remote_turn = TimeSync::saturating_sub(sample.egress_ns, sample.ingress_ns);
        const auto local_span = TimeSync::saturating_sub(sample.receive_ns, sample.origin_ns);
        const auto delay = TimeSync::saturating_sub(local_span, remote_turn);
        const auto error = TimeSync::saturating_sub(measured, offset_ns);

        integral_ns = TimeSync::clamp(
            TimeSync::saturating_add(integral_ns, error),
            -config.max_integral_ns,
            config.max_integral_ns);

        const auto p = TimeSync::shift(error, config.kp_shift);
        const auto i = TimeSync::shift(integral_ns, config.ki_shift);
        const auto correction = TimeSync::clamp(
            TimeSync::saturating_add(p, i),
            -config.max_step_ns,
            config.max_step_ns);
        offset_ns = TimeSync::saturating_add(offset_ns, correction);

        stats = PtpStats{
            .raw_offset_ns = measured,
            .filtered_offset_ns = offset_ns,
            .delay_ns = delay,
            .correction_ns = correction,
            .samples = stats.samples + 1U,
        };
        return stats;
    }
};

}  // namespace arc
