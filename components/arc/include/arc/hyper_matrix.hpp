#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

#include "arc/acoustic_slam.hpp"
#include "arc/ftm.hpp"
#include "arc/rdma.hpp"
#include "arc/result.hpp"
#include "arc/vslam.hpp"

namespace arc::swarm {

namespace detail {

[[nodiscard]] consteval std::size_t hyper_cells(
    const std::size_t x,
    const std::size_t y,
    const std::size_t z,
    const std::size_t pitch,
    const std::size_t yaw,
    const std::size_t roll) noexcept
{
    std::size_t out{1U};
    const std::array dims{x, y, z, pitch, yaw, roll};
    for (const auto dim : dims) {
        if (dim == 0U || out > (std::numeric_limits<std::size_t>::max() / dim)) {
            return 0U;
        }
        out *= dim;
    }
    return out;
}

}  // namespace detail

struct Pose6 {
    float x_mm{};
    float y_mm{};
    float z_mm{};
    float pitch_rad{};
    float yaw_rad{};
    float roll_rad{};
    std::int64_t time_us{};
};

enum class SpatialModality : std::uint8_t {
    prediction,
    visual,
    rf_ftm,
    acoustic,
};

struct HyperObservation {
    Pose6 pose{};
    float variance{};
    SpatialModality modality{SpatialModality::prediction};
};

struct HyperMatrixHeader {
    std::uint32_t cells{};
    std::uint32_t updates{};
    Pose6 estimate{};
};

template <std::size_t X, std::size_t Y, std::size_t Z, std::size_t Pitch, std::size_t Yaw, std::size_t Roll>
struct HyperMatrix {
    static_assert(X > 0U && Y > 0U && Z > 0U && Pitch > 0U && Yaw > 0U && Roll > 0U,
                  "HyperMatrix dimensions must be non-zero");
    inline constexpr static std::size_t cells = detail::hyper_cells(X, Y, Z, Pitch, Yaw, Roll);
    static_assert(cells != 0U, "HyperMatrix dimensions overflow size_t");
    static_assert(cells <= ((std::numeric_limits<std::size_t>::max() - sizeof(HyperMatrixHeader)) / sizeof(float)),
                  "HyperMatrix RDMA payload size overflows size_t");
    static_assert(cells <= std::numeric_limits<std::uint32_t>::max(),
                  "HyperMatrix cell count must fit wire metadata");

    std::array<Pose6, cells> state{};
    std::array<float, cells> probability{};
    std::uint32_t updates{};

    [[nodiscard]] static HyperMatrix seeded(const std::span<const Pose6, cells> grid) noexcept
    {
        HyperMatrix out{};
        constexpr auto prior = 1.0F / static_cast<float>(cells);
        for (std::size_t i = 0U; i < cells; ++i) {
            const auto pose = grid.data() != nullptr ? grid[i] : Pose6{};
            out.state[i] = valid_pose(pose) ? pose : Pose6{};
            out.probability[i] = prior;
        }
        return out;
    }

    [[nodiscard]] Status observe(const HyperObservation observation) noexcept
    {
        if (!std::isfinite(observation.variance) || observation.variance <= 0.0F ||
            !valid_pose(observation.pose)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto total = 0.0F;
        const auto weight = modality_weight(observation.modality);
        for (std::size_t i = 0U; i < cells; ++i) {
            const auto d = distance6(state[i], observation.pose);
            const auto likelihood = weight / (observation.variance + d);
            probability[i] = (probability[i] * 0.25F) + likelihood;
            if (!std::isfinite(probability[i])) {
                return Status{fail(ESP_ERR_INVALID_STATE)};
            }
            total += probability[i];
        }
        if (!std::isfinite(total) || total <= 0.0F) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        for (auto& value : probability) {
            value /= total;
        }
        ++updates;
        return ok();
    }

    [[nodiscard]] Result<Pose6> estimate() const noexcept
    {
        if (updates == 0U) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        Pose6 out{};
        auto total = 0.0F;
        for (std::size_t i = 0U; i < cells; ++i) {
            const auto p = probability[i];
            if (!valid_pose(state[i]) || !std::isfinite(p) || p < 0.0F) {
                return fail(ESP_ERR_INVALID_STATE);
            }
            out.x_mm += state[i].x_mm * p;
            out.y_mm += state[i].y_mm * p;
            out.z_mm += state[i].z_mm * p;
            out.pitch_rad += state[i].pitch_rad * p;
            out.yaw_rad += state[i].yaw_rad * p;
            out.roll_rad += state[i].roll_rad * p;
            out.time_us += static_cast<std::int64_t>(static_cast<float>(state[i].time_us) * p);
            total += p;
        }
        if (!std::isfinite(total) || total <= 0.0F || !valid_pose(out)) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return out;
    }

    template <typename Policy, std::size_t MaxBytes>
    [[nodiscard]] Status publish_rdma(
        const net::RdmaWindow remote,
        const std::uint32_t src,
        std::array<std::uint8_t, MaxBytes>& scratch,
        const std::uint32_t offset = 0U) const noexcept
    {
        const auto pose = estimate();
        if (!pose) {
            return Status{fail(pose.error())};
        }
        constexpr auto payload_bytes = sizeof(HyperMatrixHeader) + (cells * sizeof(float));
        if (scratch.size() < payload_bytes) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }

        const HyperMatrixHeader header{
            .cells = static_cast<std::uint32_t>(cells),
            .updates = updates,
            .estimate = *pose,
        };
        std::memcpy(scratch.data(), &header, sizeof(header));
        std::memcpy(scratch.data() + sizeof(header), probability.data(), cells * sizeof(float));
        const auto frame = net::Rdma::write<MaxBytes>(remote, src, std::span<const std::uint8_t>{scratch.data(), payload_bytes}, offset);
        if (!frame) {
            return Status{fail(frame.error())};
        }
        const auto bytes = offsetof(net::RdmaFrame<MaxBytes>, payload) + frame->header.bytes;
        return status(Policy::send(remote.node, std::span<const std::uint8_t>{
                                                    reinterpret_cast<const std::uint8_t*>(&*frame),
                                                    bytes,
                                                }));
    }

    [[nodiscard]] static constexpr HyperObservation from_vslam(
        const vision::PoseDelta delta,
        const std::int64_t time_us,
        const float variance = 50.0F) noexcept
    {
        return {
            .pose = {
                .x_mm = delta.tx_mm,
                .y_mm = delta.ty_mm,
                .z_mm = delta.tz_mm,
                .pitch_rad = delta.pitch_rad,
                .yaw_rad = delta.yaw_rad,
                .roll_rad = delta.roll_rad,
                .time_us = time_us,
            },
            .variance = variance,
            .modality = SpatialModality::visual,
        };
    }

    [[nodiscard]] static constexpr HyperObservation from_ftm(
        const Position3 position,
        const std::int64_t time_us,
        const float variance = 25.0F) noexcept
    {
        return {
            .pose = {.x_mm = position.x_mm, .y_mm = position.y_mm, .z_mm = position.z_mm, .time_us = time_us},
            .variance = variance,
            .modality = SpatialModality::rf_ftm,
        };
    }

    [[nodiscard]] static constexpr HyperObservation from_acoustic(
        const Position3 position,
        const std::int64_t time_us,
        const float variance = 100.0F) noexcept
    {
        return {
            .pose = {.x_mm = position.x_mm, .y_mm = position.y_mm, .z_mm = position.z_mm, .time_us = time_us},
            .variance = variance,
            .modality = SpatialModality::acoustic,
        };
    }

private:
    [[nodiscard]] static bool valid_pose(const Pose6 pose) noexcept
    {
        return std::isfinite(pose.x_mm) && std::isfinite(pose.y_mm) &&
            std::isfinite(pose.z_mm) && std::isfinite(pose.pitch_rad) &&
            std::isfinite(pose.yaw_rad) && std::isfinite(pose.roll_rad);
    }

    [[nodiscard]] static constexpr float modality_weight(const SpatialModality modality) noexcept
    {
        switch (modality) {
            case SpatialModality::visual:
                return 1.2F;
            case SpatialModality::rf_ftm:
                return 1.1F;
            case SpatialModality::acoustic:
                return 0.8F;
            case SpatialModality::prediction:
                return 0.5F;
        }
        return 0.5F;
    }

    [[nodiscard]] static constexpr float distance6(
        const Pose6 lhs,
        const Pose6 rhs) noexcept
    {
        const auto dx = lhs.x_mm - rhs.x_mm;
        const auto dy = lhs.y_mm - rhs.y_mm;
        const auto dz = lhs.z_mm - rhs.z_mm;
        const auto dp = (lhs.pitch_rad - rhs.pitch_rad) * 1000.0F;
        const auto dyaw = (lhs.yaw_rad - rhs.yaw_rad) * 1000.0F;
        const auto dr = (lhs.roll_rad - rhs.roll_rad) * 1000.0F;
        return (dx * dx) + (dy * dy) + (dz * dz) + (dp * dp) + (dyaw * dyaw) + (dr * dr);
    }
};

}  // namespace arc::swarm
