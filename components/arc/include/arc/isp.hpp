#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::isp {

enum class Bayer : std::uint8_t {
    rggb,
    bggr,
    grbg,
    gbrg,
};

enum class Color : std::uint8_t {
    red,
    green,
    blue,
};

[[nodiscard]] constexpr Color bayer_color(
    const Bayer pattern,
    const std::size_t x,
    const std::size_t y) noexcept
{
    const auto odd_x = (x & 1U) != 0U;
    const auto odd_y = (y & 1U) != 0U;
    switch (pattern) {
        case Bayer::rggb:
            return !odd_y ? (odd_x ? Color::green : Color::red) : (odd_x ? Color::blue : Color::green);
        case Bayer::bggr:
            return !odd_y ? (odd_x ? Color::green : Color::blue) : (odd_x ? Color::red : Color::green);
        case Bayer::grbg:
            return !odd_y ? (odd_x ? Color::red : Color::green) : (odd_x ? Color::green : Color::blue);
        case Bayer::gbrg:
            return !odd_y ? (odd_x ? Color::blue : Color::green) : (odd_x ? Color::green : Color::red);
    }
    return Color::green;
}

struct Debayer {
private:
    [[nodiscard]] static constexpr Result<std::size_t> pixels(
        const std::size_t width,
        const std::size_t height) noexcept
    {
        if (width == 0U || height == 0U || width > std::numeric_limits<std::size_t>::max() / height) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return width * height;
    }

public:
    [[nodiscard]] static Result<std::size_t> to_rgb565(
        const std::span<const std::uint8_t> raw,
        const std::size_t width,
        const std::size_t height,
        const std::span<std::uint16_t> out,
        const Bayer pattern = Bayer::rggb) noexcept
    {
        const auto total = pixels(width, height);
        if (!total || raw.size() < *total || out.size() < *total) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                std::uint32_t r{};
                std::uint32_t g{};
                std::uint32_t b{};
                std::uint32_t nr{};
                std::uint32_t ng{};
                std::uint32_t nb{};

                const auto x0 = x == 0U ? 0U : x - 1U;
                const auto y0 = y == 0U ? 0U : y - 1U;
                const auto x1 = x + 1U < width ? x + 1U : width - 1U;
                const auto y1 = y + 1U < height ? y + 1U : height - 1U;

                for (std::size_t yy = y0; yy <= y1; ++yy) {
                    for (std::size_t xx = x0; xx <= x1; ++xx) {
                        const auto sample = raw[(yy * width) + xx];
                        switch (bayer_color(pattern, xx, yy)) {
                            case Color::red:
                                r += sample;
                                ++nr;
                                break;
                            case Color::green:
                                g += sample;
                                ++ng;
                                break;
                            case Color::blue:
                                b += sample;
                                ++nb;
                                break;
                        }
                    }
                }

                out[(y * width) + x] = simd::Rgb565::pack(
                    static_cast<std::uint8_t>(nr == 0U ? 0U : r / nr),
                    static_cast<std::uint8_t>(ng == 0U ? 0U : g / ng),
                    static_cast<std::uint8_t>(nb == 0U ? 0U : b / nb));
            }
        }
        return *total;
    }
};

struct ImageStats {
    std::array<std::uint32_t, 16> histogram{};
    std::uint64_t r{};
    std::uint64_t g{};
    std::uint64_t b{};
    std::uint64_t luma{};
    std::uint32_t pixels{};
};

struct CameraTuning {
    std::int16_t exposure_delta{};
    std::uint16_t red_gain_q8{256U};
    std::uint16_t green_gain_q8{256U};
    std::uint16_t blue_gain_q8{256U};
};

struct AecAwb {
    [[nodiscard]] static ImageStats measure_rgb565(const std::span<const std::uint16_t> pixels) noexcept
    {
        ImageStats stats{};
        for (const auto pixel : pixels) {
            const auto r = static_cast<std::uint32_t>((pixel >> 11U) & 0x1fU) << 3U;
            const auto g = static_cast<std::uint32_t>((pixel >> 5U) & 0x3fU) << 2U;
            const auto b = static_cast<std::uint32_t>(pixel & 0x1fU) << 3U;
            const auto y = ((r * 77U) + (g * 150U) + (b * 29U)) >> 8U;
            ++stats.histogram[y >> 4U];
            stats.r += r;
            stats.g += g;
            stats.b += b;
            stats.luma += y;
            ++stats.pixels;
        }
        return stats;
    }

    [[nodiscard]] static CameraTuning tune(
        const ImageStats& stats,
        const std::uint8_t target_luma = 120U) noexcept
    {
        if (stats.pixels == 0U) {
            return {};
        }

        const auto avg_luma = static_cast<std::int32_t>(stats.luma / stats.pixels);
        const auto avg_r = static_cast<std::uint32_t>(stats.r / stats.pixels);
        const auto avg_g = static_cast<std::uint32_t>(stats.g / stats.pixels);
        const auto avg_b = static_cast<std::uint32_t>(stats.b / stats.pixels);
        const auto gray = (avg_r + avg_g + avg_b) / 3U;
        return {
            .exposure_delta = static_cast<std::int16_t>(static_cast<std::int32_t>(target_luma) - avg_luma),
            .red_gain_q8 = gain(gray, avg_r),
            .green_gain_q8 = gain(gray, avg_g),
            .blue_gain_q8 = gain(gray, avg_b),
        };
    }

private:
    [[nodiscard]] static std::uint16_t gain(
        const std::uint32_t target,
        const std::uint32_t value) noexcept
    {
        if (value == 0U) {
            return 256U;
        }
        const auto scaled = (target * 256U) / value;
        if (scaled < 64U) {
            return 64U;
        }
        if (scaled > 1024U) {
            return 1024U;
        }
        return static_cast<std::uint16_t>(scaled);
    }
};

}  // namespace arc::isp
