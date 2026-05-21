#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/mmu_span.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::vision {

namespace detail {

[[nodiscard]] constexpr Result<std::size_t> star_pixels(
    const std::size_t width,
    const std::size_t height) noexcept
{
    if (width == 0U || height == 0U || width > std::numeric_limits<std::size_t>::max() / height) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return width * height;
}

}  // namespace detail

struct StarPoint {
    std::uint16_t x_q4{};
    std::uint16_t y_q4{};
    std::uint16_t flux{};
};

struct StarTriangle {
    std::uint16_t a_q8{};
    std::uint16_t b_q8{};
    std::uint16_t c_q8{};
    float pitch_rad{};
    float yaw_rad{};
    float roll_rad{};
};

struct StarPose {
    float pitch_rad{};
    float yaw_rad{};
    float roll_rad{};
    std::uint32_t matches{};
};

struct StarTracker {
    [[nodiscard]] static Result<std::size_t> threshold(
        const std::span<const std::uint8_t> gray,
        const std::span<std::uint8_t> mask,
        const std::uint8_t cutoff) noexcept
    {
        if (mask.size() < gray.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto count = std::size_t{};
        auto i = std::size_t{};
        for (; i + simd::int8x16_lanes <= gray.size(); i += simd::int8x16_lanes) {
            const auto values = simd::load_unaligned<simd::uint8x16_t>(gray.data() + i);
            simd::uint8x16_t out{};
            for (std::size_t lane = 0U; lane < simd::int8x16_lanes; ++lane) {
                out[lane] = values[lane] >= cutoff ? 255U : 0U;
                count += out[lane] != 0U ? 1U : 0U;
            }
            simd::store_unaligned(mask.data() + i, out);
        }
        for (; i < gray.size(); ++i) {
            mask[i] = gray[i] >= cutoff ? 255U : 0U;
            count += mask[i] != 0U ? 1U : 0U;
        }
        return count;
    }

    [[nodiscard]] static Result<std::size_t> centroids(
        const std::span<const std::uint8_t> gray,
        const std::size_t width,
        const std::size_t height,
        const std::uint8_t cutoff,
        const std::span<StarPoint> out) noexcept
    {
        const auto total = detail::star_pixels(width, height);
        if (!total || width < 3U || height < 3U || gray.size() < *total || out.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto count = std::size_t{};
        for (std::size_t y = 1U; y + 1U < height && count < out.size(); ++y) {
            for (std::size_t x = 1U; x + 1U < width && count < out.size(); ++x) {
                const auto idx = y * width + x;
                const auto value = gray[idx];
                if (value < cutoff ||
                    value < gray[idx - 1U] ||
                    value < gray[idx + 1U] ||
                    value < gray[idx - width] ||
                    value < gray[idx + width]) {
                    continue;
                }

                std::uint32_t flux{};
                std::uint32_t sx{};
                std::uint32_t sy{};
                for (std::size_t yy = y - 1U; yy <= y + 1U; ++yy) {
                    for (std::size_t xx = x - 1U; xx <= x + 1U; ++xx) {
                        const auto sample = gray[(yy * width) + xx];
                        if (sample < cutoff) {
                            continue;
                        }
                        flux += sample;
                        sx += static_cast<std::uint32_t>(xx * 16U) * sample;
                        sy += static_cast<std::uint32_t>(yy * 16U) * sample;
                    }
                }
                if (flux == 0U) {
                    continue;
                }
                out[count++] = StarPoint{
                    .x_q4 = static_cast<std::uint16_t>(sx / flux),
                    .y_q4 = static_cast<std::uint16_t>(sy / flux),
                    .flux = static_cast<std::uint16_t>(flux > 0xFFFFU ? 0xFFFFU : flux),
                };
            }
        }
        return count;
    }

    [[nodiscard]] static Result<StarPose> match(
        const std::span<const StarPoint> observed,
        const std::span<const StarTriangle> catalog,
        const std::uint16_t tolerance_q8 = 16U) noexcept
    {
        if (observed.size() < 3U || catalog.empty()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto signature = triangle(observed[0], observed[1], observed[2]);
        for (const auto& entry : catalog) {
            if (near(signature.a_q8, entry.a_q8, tolerance_q8) &&
                near(signature.b_q8, entry.b_q8, tolerance_q8) &&
                near(signature.c_q8, entry.c_q8, tolerance_q8)) {
                return StarPose{
                    .pitch_rad = entry.pitch_rad,
                    .yaw_rad = entry.yaw_rad,
                    .roll_rad = entry.roll_rad,
                    .matches = 1U,
                };
            }
        }
        return fail(ESP_ERR_NOT_FOUND);
    }

    [[nodiscard]] static Result<StarPose> match(
        const std::span<const StarPoint> observed,
        const MmuSpan<StarTriangle>& catalog,
        const std::uint16_t tolerance_q8 = 16U) noexcept
    {
        return match(observed, catalog.values, tolerance_q8);
    }

    [[nodiscard]] static StarTriangle triangle(
        const StarPoint a,
        const StarPoint b,
        const StarPoint c) noexcept
    {
        std::array<std::uint32_t, 3> sides{
            dist_q8(a, b),
            dist_q8(b, c),
            dist_q8(c, a),
        };
        sort3(sides);
        return {
            .a_q8 = static_cast<std::uint16_t>(sides[0] > 0xFFFFU ? 0xFFFFU : sides[0]),
            .b_q8 = static_cast<std::uint16_t>(sides[1] > 0xFFFFU ? 0xFFFFU : sides[1]),
            .c_q8 = static_cast<std::uint16_t>(sides[2] > 0xFFFFU ? 0xFFFFU : sides[2]),
        };
    }

private:
    [[nodiscard]] static constexpr bool near(
        const std::uint16_t lhs,
        const std::uint16_t rhs,
        const std::uint16_t tolerance) noexcept
    {
        return lhs > rhs ? lhs - rhs <= tolerance : rhs - lhs <= tolerance;
    }

    [[nodiscard]] static std::uint32_t dist_q8(
        const StarPoint lhs,
        const StarPoint rhs) noexcept
    {
        const auto dx = static_cast<std::int32_t>(lhs.x_q4) - static_cast<std::int32_t>(rhs.x_q4);
        const auto dy = static_cast<std::int32_t>(lhs.y_q4) - static_cast<std::int32_t>(rhs.y_q4);
        const auto dist2 =
            (static_cast<std::uint64_t>(dx < 0 ? -dx : dx) * static_cast<std::uint64_t>(dx < 0 ? -dx : dx)) +
            (static_cast<std::uint64_t>(dy < 0 ? -dy : dy) * static_cast<std::uint64_t>(dy < 0 ? -dy : dy));
        return isqrt(dist2) << 4U;
    }

    [[nodiscard]] static std::uint32_t isqrt(const std::uint64_t value) noexcept
    {
        auto root = std::uint64_t{};
        auto bit = std::uint64_t{1} << 62U;
        auto n = value;
        while (bit > n) {
            bit >>= 2U;
        }
        while (bit != 0U) {
            if (n >= root + bit) {
                n -= root + bit;
                root = (root >> 1U) + bit;
            } else {
                root >>= 1U;
            }
            bit >>= 2U;
        }
        return root > 0xFFFF'FFFFULL ? 0xFFFF'FFFFU : static_cast<std::uint32_t>(root);
    }

    static constexpr void sort3(std::array<std::uint32_t, 3>& values) noexcept
    {
        if (values[1] < values[0]) {
            swap(values[0], values[1]);
        }
        if (values[2] < values[1]) {
            swap(values[1], values[2]);
        }
        if (values[1] < values[0]) {
            swap(values[0], values[1]);
        }
    }

    static constexpr void swap(std::uint32_t& lhs, std::uint32_t& rhs) noexcept
    {
        const auto tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }
};

}  // namespace arc::vision
