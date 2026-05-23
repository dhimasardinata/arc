#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/foc.hpp"
#include "arc/result.hpp"

namespace arc::vision {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool vision_valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

[[nodiscard]] constexpr Result<std::size_t> pixels(
    const std::size_t width,
    const std::size_t height) noexcept
{
    if (width == 0U || height == 0U || width > std::numeric_limits<std::size_t>::max() / height) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return width * height;
}

}  // namespace detail

struct MotionVector {
    std::int32_t dx_q8{};
    std::int32_t dy_q8{};
    std::uint32_t confidence{};
};

struct Sobel {
    [[nodiscard]] static Result<std::size_t> edges(
        const std::span<const std::uint8_t> gray,
        const std::size_t width,
        const std::size_t height,
        const std::span<std::uint8_t> out,
        const std::uint16_t threshold = 0U) noexcept
    {
        const auto total = detail::pixels(width, height);
        if (!total || width < 3U || height < 3U || !detail::vision_valid_span(gray) ||
            !detail::vision_valid_span(out) || gray.size() < *total || out.size() < *total) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        for (auto& value : out.first(*total)) {
            value = 0U;
        }

        for (std::size_t y = 1U; y + 1U < height; ++y) {
            for (std::size_t x = 1U; x + 1U < width; ++x) {
                const auto idx = y * width + x;
                const auto gx =
                    -static_cast<int>(gray[idx - width - 1U]) +
                    static_cast<int>(gray[idx - width + 1U]) -
                    (2 * static_cast<int>(gray[idx - 1U])) +
                    (2 * static_cast<int>(gray[idx + 1U])) -
                    static_cast<int>(gray[idx + width - 1U]) +
                    static_cast<int>(gray[idx + width + 1U]);
                const auto gy =
                    -static_cast<int>(gray[idx - width - 1U]) -
                    (2 * static_cast<int>(gray[idx - width])) -
                    static_cast<int>(gray[idx - width + 1U]) +
                    static_cast<int>(gray[idx + width - 1U]) +
                    (2 * static_cast<int>(gray[idx + width])) +
                    static_cast<int>(gray[idx + width + 1U]);
                const auto mag = abs_i(gx) + abs_i(gy);
                out[idx] = mag > static_cast<int>(threshold) ? clamp_u8(mag) : 0U;
            }
        }
        return *total;
    }

private:
    [[nodiscard]] static constexpr int abs_i(const int value) noexcept
    {
        return value < 0 ? -value : value;
    }

    [[nodiscard]] static constexpr std::uint8_t clamp_u8(const int value) noexcept
    {
        return value > 255 ? 255U : static_cast<std::uint8_t>(value);
    }
};

struct OpticalFlow {
    [[nodiscard]] static Result<MotionVector> lucas_kanade(
        const std::span<const std::uint8_t> previous,
        const std::span<const std::uint8_t> current,
        const std::size_t width,
        const std::size_t height) noexcept
    {
        const auto total = detail::pixels(width, height);
        if (!total || width < 3U || height < 3U || !detail::vision_valid_span(previous) ||
            !detail::vision_valid_span(current) || previous.size() < *total || current.size() < *total) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        double gxx{};
        double gxy{};
        double gyy{};
        double gxt{};
        double gyt{};

        for (std::size_t y = 1U; y + 1U < height; ++y) {
            for (std::size_t x = 1U; x + 1U < width; ++x) {
                const auto idx = y * width + x;
                const auto ix = static_cast<int>(current[idx + 1U]) - static_cast<int>(current[idx - 1U]);
                const auto iy = static_cast<int>(current[idx + width]) - static_cast<int>(current[idx - width]);
                const auto it = static_cast<int>(current[idx]) - static_cast<int>(previous[idx]);
                gxx += ix * ix;
                gxy += ix * iy;
                gyy += iy * iy;
                gxt += ix * it;
                gyt += iy * it;
            }
        }

        const auto det = (gxx * gyy) - (gxy * gxy);
        if (!std::isfinite(det)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (det == 0.0) {
            return MotionVector{};
        }

        const auto dx = ((gxy * gyt) - (gyy * gxt)) * 256.0 / det;
        const auto dy = ((gxy * gxt) - (gxx * gyt)) * 256.0 / det;
        if (!std::isfinite(dx) || !std::isfinite(dy)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return MotionVector{
            .dx_q8 = clamp_i32(dx),
            .dy_q8 = clamp_i32(dy),
            .confidence = clamp_u32_abs(det),
        };
    }

private:
    [[nodiscard]] static constexpr std::int32_t clamp_i32(const double value) noexcept
    {
        if (value >= static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
            return std::numeric_limits<std::int32_t>::max();
        }
        if (value <= static_cast<double>(std::numeric_limits<std::int32_t>::min())) {
            return std::numeric_limits<std::int32_t>::min();
        }
        return static_cast<std::int32_t>(value);
    }

    [[nodiscard]] static constexpr std::uint32_t clamp_u32_abs(const double value) noexcept
    {
        const auto magnitude = value < 0.0 ? -value : value;
        if (magnitude >= static_cast<double>(std::numeric_limits<std::uint32_t>::max())) {
            return std::numeric_limits<std::uint32_t>::max();
        }
        return static_cast<std::uint32_t>(magnitude);
    }
};

struct VisualServo {
    [[nodiscard]] static FocTarget<float> flow_target(
        const MotionVector flow,
        const float gain,
        const float bus) noexcept
    {
        return {
            .d = 0.0F,
            .q = -(static_cast<float>(flow.dx_q8) / 256.0F) * gain,
            .bus = bus,
        };
    }
};

}  // namespace arc::vision
