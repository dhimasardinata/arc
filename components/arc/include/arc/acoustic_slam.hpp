#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/covert.hpp"
#include "arc/dsp.hpp"
#include "arc/kalman.hpp"
#include "arc/result.hpp"
#include "arc/timesync.hpp"

namespace arc::swarm {

struct Position3 {
    float x_mm{};
    float y_mm{};
    float z_mm{};
};

struct AcousticAnchor {
    std::uint32_t node_id{};
    Position3 position{};
};

struct ChirpConfig {
    std::uint32_t start_hz{18'000U};
    std::uint32_t stop_hz{22'000U};
    std::uint32_t sample_hz{96'000U};
    std::int16_t amplitude{12'000};
};

struct TdoaEstimate {
    std::int32_t lag_samples{};
    float delta_mm{};
    float confidence{};
};

struct AcousticSlam {
    [[nodiscard]] static constexpr bool chirp_valid(const ChirpConfig config) noexcept
    {
        return config.start_hz != 0U &&
            config.stop_hz != 0U &&
            below_nyquist(config.start_hz, config.sample_hz) &&
            below_nyquist(config.stop_hz, config.sample_hz) &&
            config.amplitude > 0;
    }

    [[nodiscard]] static Result<std::size_t> fmcw_chirp(
        const std::span<std::int16_t> out,
        const ChirpConfig config = {}) noexcept
    {
        if (out.empty() || out.data() == nullptr || !chirp_valid(config)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto phase = 0.0F;
        constexpr auto two_pi = 6.28318530717958647692F;
        for (std::size_t i = 0U; i < out.size(); ++i) {
            const auto mix = out.size() == 1U ? 0.0F : static_cast<float>(i) / static_cast<float>(out.size() - 1U);
            const auto hz = static_cast<float>(config.start_hz) +
                ((static_cast<float>(config.stop_hz) - static_cast<float>(config.start_hz)) * mix);
            phase += two_pi * hz / static_cast<float>(config.sample_hz);
            out[i] = static_cast<std::int16_t>(std::sin(phase) * static_cast<float>(config.amplitude));
        }
        return out.size();
    }

    [[nodiscard]] static Result<TdoaEstimate> tdoa(
        const std::span<const std::int16_t> reference,
        const std::span<const std::int16_t> delayed,
        const std::uint32_t sample_hz,
        const std::size_t max_lag,
        const float sound_mmps = 343'000.0F) noexcept
    {
        constexpr auto max_scan_lag = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max() - 1);
        if (reference.empty() || delayed.empty() || reference.data() == nullptr || delayed.data() == nullptr ||
            sample_hz == 0U || max_lag > max_scan_lag || !std::isfinite(sound_mmps) || sound_mmps <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto best_lag = std::int32_t{};
        auto best_score = std::int64_t{};
        auto best_ready = false;
        const auto signed_max = static_cast<std::int32_t>(max_lag);
        for (auto lag = -signed_max; lag <= signed_max; ++lag) {
            auto acc = std::int64_t{};
            auto count = std::size_t{};
            for (std::size_t i = 0U; i < reference.size(); ++i) {
                const auto j_signed = static_cast<std::int64_t>(i) + lag;
                if (j_signed < 0 || static_cast<std::uint64_t>(j_signed) >= delayed.size()) {
                    continue;
                }
                acc += static_cast<std::int32_t>(reference[i]) *
                    static_cast<std::int32_t>(delayed[static_cast<std::size_t>(j_signed)]);
                ++count;
            }
            if (count == 0U) {
                continue;
            }
            const auto score = acc < 0 ? -acc : acc;
            if (!best_ready || score > best_score) {
                best_ready = true;
                best_score = score;
                best_lag = lag;
            }
        }
        if (!best_ready) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        const auto dt_s = static_cast<float>(best_lag) / static_cast<float>(sample_hz);
        return TdoaEstimate{
            .lag_samples = best_lag,
            .delta_mm = dt_s * sound_mmps,
            .confidence = static_cast<float>(best_score),
        };
    }

    [[nodiscard]] static Result<TdoaEstimate> gcc_tdoa(
        const std::span<simd::ComplexF32> reference,
        const std::span<simd::ComplexF32> delayed,
        const std::span<simd::ComplexF32> scratch,
        const std::size_t max_lag,
        const float sample_hz,
        const float sound_mmps = 343'000.0F) noexcept
    {
        if (!valid_span(reference) || !valid_span(delayed) || !valid_span(scratch) ||
            !std::isfinite(sample_hz) || sample_hz <= 0.0F ||
            !std::isfinite(sound_mmps) || sound_mmps <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto estimate = dsp::Beamform<float, 2>::gcc_phat(
            reference,
            delayed,
            scratch,
            max_lag,
            sample_hz,
            1.0F,
            sound_mmps / 1000.0F);
        if (!estimate) {
            return fail(estimate.error());
        }
        const auto dt_s = static_cast<float>(estimate->lag_samples) / sample_hz;
        return TdoaEstimate{
            .lag_samples = estimate->lag_samples,
            .delta_mm = dt_s * sound_mmps,
            .confidence = estimate->confidence,
        };
    }

    [[nodiscard]] static Result<float> sync_range(
        const TimeSync& clock,
        const std::int64_t local_rx_us,
        const std::int64_t remote_tx_us,
        const float sound_mmps = 343'000.0F) noexcept
    {
        if (!std::isfinite(sound_mmps) || sound_mmps <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto tx_local = clock.to_local(remote_tx_us);
        const auto flight_us = TimeSync::sat_sub(local_rx_us, tx_local);
        if (flight_us < 0) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return (static_cast<float>(flight_us) * sound_mmps) / 1'000'000.0F;
    }

    template <std::size_t Anchors>
    [[nodiscard]] static Result<Position3> solve(
        const std::span<const AcousticAnchor, Anchors> anchors,
        const std::span<const float, Anchors> ranges_mm) noexcept
    {
        static_assert(Anchors >= 4U, "3D acoustic SLAM needs at least four anchors");
        if (!valid_geometry(anchors, ranges_mm)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        double ata[3][3]{};
        double atb[3]{};
        const auto origin = anchors[0].position;
        const auto r0 = static_cast<double>(ranges_mm[0]);
        for (std::size_t i = 1U; i < Anchors; ++i) {
            const auto p = anchors[i].position;
            const double row[3]{
                2.0 * (static_cast<double>(p.x_mm) - static_cast<double>(origin.x_mm)),
                2.0 * (static_cast<double>(p.y_mm) - static_cast<double>(origin.y_mm)),
                2.0 * (static_cast<double>(p.z_mm) - static_cast<double>(origin.z_mm)),
            };
            const auto range = static_cast<double>(ranges_mm[i]);
            const auto b =
                (r0 * r0) - (range * range) -
                (static_cast<double>(origin.x_mm) * static_cast<double>(origin.x_mm)) +
                (static_cast<double>(p.x_mm) * static_cast<double>(p.x_mm)) -
                (static_cast<double>(origin.y_mm) * static_cast<double>(origin.y_mm)) +
                (static_cast<double>(p.y_mm) * static_cast<double>(p.y_mm)) -
                (static_cast<double>(origin.z_mm) * static_cast<double>(origin.z_mm)) +
                (static_cast<double>(p.z_mm) * static_cast<double>(p.z_mm));

            for (std::size_t r = 0U; r < 3U; ++r) {
                atb[r] += row[r] * b;
                for (std::size_t c = 0U; c < 3U; ++c) {
                    ata[r][c] += row[r] * row[c];
                }
            }
        }
        return solve3(ata, atb);
    }

    [[nodiscard]] static Status correct_filter(
        dsp::Kalman<float, 6, 3>& filter,
        const Position3 measurement,
        const float noise_mm = 25.0F) noexcept
    {
        if (!finite(measurement) || !std::isfinite(noise_mm) || noise_mm <= 0.0F) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        dsp::Kalman<float, 6, 3>::H h{};
        dsp::Kalman<float, 6, 3>::R r{};
        dsp::Kalman<float, 6, 3>::Measure z{};
        h(0, 0) = 1.0F;
        h(1, 1) = 1.0F;
        h(2, 2) = 1.0F;
        r(0, 0) = noise_mm;
        r(1, 1) = noise_mm;
        r(2, 2) = noise_mm;
        z(0, 0) = measurement.x_mm;
        z(1, 0) = measurement.y_mm;
        z(2, 0) = measurement.z_mm;
        filter.correct_diagonal(h, r, z);
        return ok();
    }

    template <typename Sonic>
    [[nodiscard]] static Status emit_chirp(const covert::FskConfig config) noexcept
    {
        return Sonic::bit(true, config);
    }

private:
    [[nodiscard]] static constexpr bool below_nyquist(
        const std::uint32_t hz,
        const std::uint32_t sample_hz) noexcept
    {
        return sample_hz > 1U && hz <= (sample_hz - 1U) / 2U;
    }

    template <typename T>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static bool finite(const Position3 value) noexcept
    {
        return std::isfinite(value.x_mm) && std::isfinite(value.y_mm) && std::isfinite(value.z_mm);
    }

    template <std::size_t Anchors>
    [[nodiscard]] static bool valid_geometry(
        const std::span<const AcousticAnchor, Anchors> anchors,
        const std::span<const float, Anchors> ranges_mm) noexcept
    {
        for (std::size_t i = 0U; i < Anchors; ++i) {
            if (!finite(anchors[i].position) || !std::isfinite(ranges_mm[i]) || ranges_mm[i] < 0.0F) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static Result<Position3> solve3(
        double a[3][3],
        double b[3]) noexcept
    {
        for (std::size_t pivot = 0U; pivot < 3U; ++pivot) {
            auto best = pivot;
            auto best_abs = abs(a[pivot][pivot]);
            for (std::size_t row = pivot + 1U; row < 3U; ++row) {
                const auto value = abs(a[row][pivot]);
                if (value > best_abs) {
                    best = row;
                    best_abs = value;
                }
            }
            if (best_abs < 0.000000000001) {
                return fail(ESP_ERR_INVALID_STATE);
            }
            if (best != pivot) {
                for (std::size_t col = pivot; col < 3U; ++col) {
                    const auto tmp = a[pivot][col];
                    a[pivot][col] = a[best][col];
                    a[best][col] = tmp;
                }
                const auto tmp = b[pivot];
                b[pivot] = b[best];
                b[best] = tmp;
            }

            const auto div = a[pivot][pivot];
            for (std::size_t col = pivot; col < 3U; ++col) {
                a[pivot][col] /= div;
            }
            b[pivot] /= div;

            for (std::size_t row = 0U; row < 3U; ++row) {
                if (row == pivot) {
                    continue;
                }
                const auto scale = a[row][pivot];
                for (std::size_t col = pivot; col < 3U; ++col) {
                    a[row][col] -= scale * a[pivot][col];
                }
                b[row] -= scale * b[pivot];
            }
        }
        return Position3{
            .x_mm = static_cast<float>(b[0]),
            .y_mm = static_cast<float>(b[1]),
            .z_mm = static_cast<float>(b[2]),
        };
    }

    [[nodiscard]] static constexpr double abs(const double value) noexcept
    {
        return value < 0.0 ? -value : value;
    }
};

}  // namespace arc::swarm
