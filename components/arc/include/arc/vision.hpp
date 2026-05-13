#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/foc.hpp"
#include "arc/result.hpp"

namespace arc::vision {

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
        if (width < 3U || height < 3U || gray.size() < width * height || out.size() < width * height) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        for (auto& value : out.first(width * height)) {
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
        return width * height;
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
        if (width < 3U || height < 3U || previous.size() < width * height || current.size() < width * height) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::int64_t gxx{};
        std::int64_t gxy{};
        std::int64_t gyy{};
        std::int64_t gxt{};
        std::int64_t gyt{};

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
        if (det == 0) {
            return MotionVector{};
        }

        const auto dx = ((gxy * gyt) - (gyy * gxt)) * 256 / det;
        const auto dy = ((gxy * gxt) - (gxx * gyt)) * 256 / det;
        return MotionVector{
            .dx_q8 = static_cast<std::int32_t>(dx),
            .dy_q8 = static_cast<std::int32_t>(dy),
            .confidence = static_cast<std::uint32_t>(det > 0 ? det : -det),
        };
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
