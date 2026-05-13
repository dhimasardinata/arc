#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/acoustic_slam.hpp"
#include "arc/dsp.hpp"
#include "arc/result.hpp"
#include "arc/swarm.hpp"

#if __has_include("esp_wifi.h")
#include "esp_wifi.h"
#endif

namespace arc::net {

struct FtmPeer {
    std::uint32_t node_id{};
    std::array<std::uint8_t, 6> mac{};
    swarm::Position3 position{};
    std::uint8_t channel{};
};

struct FtmSessionConfig {
    std::uint8_t frames{32U};
    std::uint16_t burst_period_100ms{2U};
    std::uint32_t timeout_ms{8000U};
};

struct FtmMeasurement {
    std::uint32_t node_id{};
    std::uint32_t rtt_ns{};
    float distance_mm{};
};

struct Ftm {
    template <typename Policy>
    [[nodiscard]] static Status initiate(
        const FtmPeer peer,
        const FtmSessionConfig config = {}) noexcept
    {
        if (peer.node_id == 0U || peer.channel == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::ftm_initiate(peer, config));
    }

    [[nodiscard]] static constexpr FtmMeasurement measurement(
        const std::uint32_t node_id,
        const std::uint32_t rtt_ns) noexcept
    {
        constexpr auto light_mmps = 299'792'458'000.0F;
        return {
            .node_id = node_id,
            .rtt_ns = rtt_ns,
            .distance_mm = (static_cast<float>(rtt_ns) * light_mmps) / 2'000'000'000.0F,
        };
    }

    template <std::size_t Anchors>
    [[nodiscard]] static Result<swarm::Position3> trilaterate(
        const std::span<const FtmPeer, Anchors> peers,
        const std::span<const FtmMeasurement, Anchors> ranges,
        const SwarmSchedule<Anchors>* const schedule = nullptr,
        const std::uint64_t now_us = 0U) noexcept
    {
        static_assert(Anchors >= 4U, "3D RF FTM positioning needs at least four peers");

        if (schedule != nullptr) {
            for (std::size_t i = 0U; i < Anchors; ++i) {
                if (!schedule->active(peers[i].node_id, now_us)) {
                    return fail(ESP_ERR_INVALID_STATE);
                }
            }
        }

        dsp::Matrix<float, 3, 3> ata{};
        dsp::Matrix<float, 3, 1> atb{};
        const auto origin = peers[0].position;
        const auto r0 = ranges[0].distance_mm;
        if (peers[0].node_id != ranges[0].node_id) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (std::size_t i = 1U; i < Anchors; ++i) {
            if (peers[i].node_id != ranges[i].node_id) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            const auto p = peers[i].position;
            const float row[3]{
                2.0F * (p.x_mm - origin.x_mm),
                2.0F * (p.y_mm - origin.y_mm),
                2.0F * (p.z_mm - origin.z_mm),
            };
            const auto b =
                (r0 * r0) - (ranges[i].distance_mm * ranges[i].distance_mm) -
                (origin.x_mm * origin.x_mm) + (p.x_mm * p.x_mm) -
                (origin.y_mm * origin.y_mm) + (p.y_mm * p.y_mm) -
                (origin.z_mm * origin.z_mm) + (p.z_mm * p.z_mm);

            for (std::size_t r = 0U; r < 3U; ++r) {
                atb(r, 0) += row[r] * b;
                for (std::size_t c = 0U; c < 3U; ++c) {
                    ata(r, c) += row[r] * row[c];
                }
            }
        }
        return solve3(ata, atb);
    }

private:
    [[nodiscard]] static Result<swarm::Position3> solve3(
        dsp::Matrix<float, 3, 3> a,
        dsp::Matrix<float, 3, 1> b) noexcept
    {
        for (std::size_t pivot = 0U; pivot < 3U; ++pivot) {
            auto best = pivot;
            auto best_abs = abs(a(pivot, pivot));
            for (std::size_t row = pivot + 1U; row < 3U; ++row) {
                const auto value = abs(a(row, pivot));
                if (value > best_abs) {
                    best = row;
                    best_abs = value;
                }
            }
            if (best_abs < 0.000001F) {
                return fail(ESP_ERR_INVALID_STATE);
            }
            if (best != pivot) {
                for (std::size_t col = pivot; col < 3U; ++col) {
                    const auto tmp = a(pivot, col);
                    a(pivot, col) = a(best, col);
                    a(best, col) = tmp;
                }
                const auto tmp = b(pivot, 0);
                b(pivot, 0) = b(best, 0);
                b(best, 0) = tmp;
            }

            const auto div = a(pivot, pivot);
            for (std::size_t col = pivot; col < 3U; ++col) {
                a(pivot, col) /= div;
            }
            b(pivot, 0) /= div;

            for (std::size_t row = 0U; row < 3U; ++row) {
                if (row == pivot) {
                    continue;
                }
                const auto scale = a(row, pivot);
                for (std::size_t col = pivot; col < 3U; ++col) {
                    a(row, col) -= scale * a(pivot, col);
                }
                b(row, 0) -= scale * b(pivot, 0);
            }
        }
        return swarm::Position3{.x_mm = b(0, 0), .y_mm = b(1, 0), .z_mm = b(2, 0)};
    }

    [[nodiscard]] static constexpr float abs(const float value) noexcept
    {
        return value < 0.0F ? -value : value;
    }
};

#if __has_include("esp_wifi.h")
struct EspWifiFtmPolicy {
    [[nodiscard]] static esp_err_t ftm_initiate(
        const FtmPeer& peer,
        const FtmSessionConfig config) noexcept
    {
        wifi_ftm_initiator_cfg_t native{};
        for (std::size_t i = 0U; i < peer.mac.size(); ++i) {
            native.resp_mac[i] = peer.mac[i];
        }
        native.channel = peer.channel;
        native.frm_count = config.frames;
        native.burst_period = config.burst_period_100ms;
        return esp_wifi_ftm_initiate_session(&native);
    }
};
#endif

}  // namespace arc::net
