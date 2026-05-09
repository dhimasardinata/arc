#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dsp.hpp"
#include "arc/nav.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::vision {

struct Corner {
    std::uint16_t x{};
    std::uint16_t y{};
    std::uint8_t score{};
};

struct PoseDelta {
    float tx_mm{};
    float ty_mm{};
    float tz_mm{};
    float yaw_rad{};
    float pitch_rad{};
    float roll_rad{};
    std::uint32_t tracks{};
};

struct EssentialEstimate {
    dsp::Matrix<float, 3, 3> essential{};
    PoseDelta delta{};
    std::uint32_t inliers{};
};

struct VSlam {
    inline constexpr static std::size_t fast_lanes = simd::int8x16_lanes;

    template <typename Camera>
    [[nodiscard]] static Result<std::span<std::uint8_t>> capture_gray(
        const std::span<std::uint8_t> frame) noexcept
    {
        if (frame.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto err = Camera::capture_gray(frame);
        if (err != ESP_OK) {
            return fail(err);
        }
        return frame;
    }

    [[nodiscard]] static Result<std::span<Corner>> fast_corners(
        const std::span<const std::uint8_t> gray,
        const std::size_t width,
        const std::size_t height,
        const std::span<Corner> out,
        const std::uint8_t threshold = 20U) noexcept
    {
        if (width < 7U || height < 7U || gray.size() < width * height || out.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto found = std::size_t{};
        for (std::size_t y = 3U; y + 3U < height; ++y) {
            for (std::size_t x = 3U; x + 3U < width; x += fast_lanes) {
                const auto run = (x + fast_lanes + 3U <= width) ? fast_lanes : (width - 3U - x);
                std::array<std::uint8_t, fast_lanes> center_bytes{};
                for (std::size_t lane = 0U; lane < run; ++lane) {
                    center_bytes[lane] = gray[(y * width) + x + lane];
                }
                const auto center = simd::load_unaligned<simd::uint8x16_t>(center_bytes.data());
                for (std::size_t lane = 0U; lane < run; ++lane) {
                    const auto score = fast_score(gray, width, x + lane, y, center[lane], threshold);
                    if (score < 9U) {
                        continue;
                    }
                    if (found >= out.size()) {
                        return fail(ESP_ERR_NO_MEM);
                    }
                    out[found++] = Corner{
                        .x = static_cast<std::uint16_t>(x + lane),
                        .y = static_cast<std::uint16_t>(y),
                        .score = score,
                    };
                }
            }
        }
        return out.first(found);
    }

    [[nodiscard]] static Result<EssentialEstimate> essential_from_flow(
        const std::span<const Corner> previous,
        const std::span<const Corner> current,
        const std::size_t tracks,
        const float focal_px = 160.0F) noexcept
    {
        const auto count = min(previous.size(), min(current.size(), tracks));
        if (count == 0U || focal_px <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        float dx{};
        float dy{};
        for (std::size_t i = 0U; i < count; ++i) {
            dx += static_cast<float>(static_cast<int>(current[i].x) - static_cast<int>(previous[i].x));
            dy += static_cast<float>(static_cast<int>(current[i].y) - static_cast<int>(previous[i].y));
        }
        dx /= static_cast<float>(count);
        dy /= static_cast<float>(count);

        const auto tx = dx / focal_px;
        const auto ty = dy / focal_px;
        const auto tz = 1.0F;
        dsp::Matrix<float, 3, 3> essential{};
        essential(0, 1) = -tz;
        essential(0, 2) = ty;
        essential(1, 0) = tz;
        essential(1, 2) = -tx;
        essential(2, 0) = -ty;
        essential(2, 1) = tx;

        return EssentialEstimate{
            .essential = essential,
            .delta = {
                .tx_mm = tx * 1000.0F,
                .ty_mm = ty * 1000.0F,
                .tz_mm = tz,
                .yaw_rad = tx,
                .pitch_rad = ty,
                .roll_rad = 0.0F,
                .tracks = static_cast<std::uint32_t>(count),
            },
            .inliers = static_cast<std::uint32_t>(count),
        };
    }

    [[nodiscard]] static Status correct_filter(
        nav::Eskf<float>::State& state,
        const PoseDelta delta,
        const float gain = 0.25F) noexcept
    {
        const nav::Eskf<float>::Vec3 position{
            state.position[0] + (delta.tx_mm * 0.001F),
            state.position[1] + (delta.ty_mm * 0.001F),
            state.position[2] + (delta.tz_mm * 0.001F),
        };
        const nav::Quaternion<float> attitude{
            .w = 1.0F,
            .x = delta.roll_rad * 0.5F,
            .y = delta.pitch_rad * 0.5F,
            .z = delta.yaw_rad * 0.5F,
        };
        return nav::Eskf<float>::correct_pose(state, position, attitude, gain);
    }

private:
    [[nodiscard]] static std::uint8_t fast_score(
        const std::span<const std::uint8_t> gray,
        const std::size_t width,
        const std::size_t x,
        const std::size_t y,
        const std::uint8_t center,
        const std::uint8_t threshold) noexcept
    {
        constexpr std::array<std::array<int, 2>, 16> circle{{
            {{0, -3}},
            {{1, -3}},
            {{2, -2}},
            {{3, -1}},
            {{3, 0}},
            {{3, 1}},
            {{2, 2}},
            {{1, 3}},
            {{0, 3}},
            {{-1, 3}},
            {{-2, 2}},
            {{-3, 1}},
            {{-3, 0}},
            {{-3, -1}},
            {{-2, -2}},
            {{-1, -3}},
        }};

        auto brighter = std::uint8_t{};
        auto darker = std::uint8_t{};
        auto bright_run = std::uint8_t{};
        auto dark_run = std::uint8_t{};
        auto best_bright = std::uint8_t{};
        auto best_dark = std::uint8_t{};
        for (std::size_t i = 0U; i < circle.size() * 2U; ++i) {
            const auto& offset = circle[i % circle.size()];
            const auto yy = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(y) + offset[1]);
            const auto xx = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(x) + offset[0]);
            const auto value = gray[(yy * width) + xx];
            brighter = value > center && value - center > threshold ? 1U : 0U;
            darker = center > value && center - value > threshold ? 1U : 0U;
            bright_run = brighter != 0U ? static_cast<std::uint8_t>(bright_run + 1U) : 0U;
            dark_run = darker != 0U ? static_cast<std::uint8_t>(dark_run + 1U) : 0U;
            best_bright = bright_run > best_bright ? bright_run : best_bright;
            best_dark = dark_run > best_dark ? dark_run : best_dark;
        }
        const auto best = best_bright > best_dark ? best_bright : best_dark;
        return best > circle.size() ? static_cast<std::uint8_t>(circle.size()) : best;
    }

    [[nodiscard]] static constexpr std::size_t min(
        const std::size_t lhs,
        const std::size_t rhs) noexcept
    {
        return lhs < rhs ? lhs : rhs;
    }
};

}  // namespace arc::vision
