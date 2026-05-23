#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <system_error>

#include "arc/motion.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"
#include "arc/stream.hpp"

namespace arc::cnc {

enum class Command : std::uint8_t {
    none,
    rapid,
    linear,
    dwell,
    home,
    spindle,
};

struct GCodeBlock {
    Command command{Command::none};
    std::array<float, 6> axis{};
    std::array<bool, 6> has_axis{};
    float feed{};
    bool has_feed{};
    std::uint32_t line{};
};

struct CoreXYConfig {
    float steps_mm{80.0F};
    std::uint32_t ticks_step{1U};
};

struct DeltaConfig {
    float arm_mm{220.0F};
    float radius_mm{90.0F};
    float steps_mm{80.0F};
    std::uint32_t ticks_step{1U};
};

struct Kinematics {
    [[nodiscard]] static MotionSegment<2> corexy(
        const float x_mm,
        const float y_mm,
        const CoreXYConfig config = {}) noexcept
    {
        return {
            .delta = {
                round_i32((x_mm + y_mm) * config.steps_mm),
                round_i32((x_mm - y_mm) * config.steps_mm),
            },
            .ticks_step = config.ticks_step,
        };
    }

    [[nodiscard]] static Result<MotionSegment<3>> delta(
        const std::array<float, 3> xyz_mm,
        const DeltaConfig config = {}) noexcept
    {
        if (!finite(config.arm_mm) || !finite(config.radius_mm) || !finite(config.steps_mm) ||
            config.arm_mm <= 0.0F || config.radius_mm <= 0.0F || config.steps_mm <= 0.0F ||
            !finite(xyz_mm[0]) || !finite(xyz_mm[1]) || !finite(xyz_mm[2])) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        constexpr std::array<float, 3> tower_x{1.0F, -0.5F, -0.5F};
        constexpr std::array<float, 3> tower_y{0.0F, 0.8660254038F, -0.8660254038F};
        MotionSegment<3> out{.ticks_step = config.ticks_step};
        for (std::size_t i = 0U; i < 3U; ++i) {
            const auto dx = xyz_mm[0] - (tower_x[i] * config.radius_mm);
            const auto dy = xyz_mm[1] - (tower_y[i] * config.radius_mm);
            const auto reach = (config.arm_mm * config.arm_mm) - (dx * dx) - (dy * dy);
            if (reach <= 0.0F) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            out.delta[i] = round_i32((xyz_mm[2] + std::sqrt(reach)) * config.steps_mm);
        }
        return out;
    }

    [[nodiscard]] static MotionSegment<5> five_axis(
        const std::array<float, 5> target,
        const std::array<float, 5> steps_unit,
        const std::uint32_t ticks_step = 1U) noexcept
    {
        MotionSegment<5> out{.ticks_step = ticks_step};
        const simd::float32x4_t target4{target[0], target[1], target[2], target[3]};
        const simd::float32x4_t scale4{steps_unit[0], steps_unit[1], steps_unit[2], steps_unit[3]};
        const auto steps4 = target4 * scale4;
        for (std::size_t i = 0U; i < 4U; ++i) {
            out.delta[i] = round_i32(steps4[i]);
        }
        out.delta[4] = round_i32(target[4] * steps_unit[4]);
        return out;
    }

    [[nodiscard]] static std::int32_t round_i32(const float value) noexcept
    {
        if (!finite(value)) {
            return value < 0.0F ? std::numeric_limits<std::int32_t>::min()
                : value > 0.0F  ? std::numeric_limits<std::int32_t>::max()
                                : 0;
        }
        const auto widened = static_cast<double>(value);
        constexpr auto hi = static_cast<double>(std::numeric_limits<std::int32_t>::max()) - 0.5;
        constexpr auto lo = static_cast<double>(std::numeric_limits<std::int32_t>::min()) + 0.5;
        if (widened >= hi) {
            return std::numeric_limits<std::int32_t>::max();
        }
        if (widened <= lo) {
            return std::numeric_limits<std::int32_t>::min();
        }
        return static_cast<std::int32_t>(value >= 0.0F ? value + 0.5F : value - 0.5F);
    }

private:
    [[nodiscard]] static bool finite(const float value) noexcept
    {
        return std::isfinite(value);
    }
};

struct GCode {
    [[nodiscard]] static Result<GCodeBlock> parse_line(const std::span<const char> line) noexcept
    {
        if (line.empty() || !valid_span(line)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        GCodeBlock out{};
        for (std::size_t i = 0U; i < line.size();) {
            while (i < line.size() && is_space(line[i])) {
                ++i;
            }
            if (i >= line.size() || line[i] == ';') {
                break;
            }
            const auto word = upper(line[i++]);
            const auto start = i;
            while (i < line.size() && !is_space(line[i]) && line[i] != ';') {
                ++i;
            }
            float value{};
            const auto parsed = std::from_chars(line.data() + start, line.data() + i, value);
            if (parsed.ec != std::errc{} || !finite(value)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (word == 'G') {
                if (!fits_i32(value)) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                apply_g(out, static_cast<int>(value));
            } else if (word == 'M') {
                out.command = Command::spindle;
            } else if (word == 'F') {
                out.feed = value;
                out.has_feed = true;
            } else if (word == 'N') {
                if (!fits_u32(value)) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                out.line = static_cast<std::uint32_t>(value);
            } else {
                const auto axis = axis_index(word);
                if (axis >= out.axis.size()) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
                out.axis[axis] = value;
                out.has_axis[axis] = true;
            }
        }
        return out;
    }

    template <net::ByteStream Io>
    [[nodiscard]] static Result<GCodeBlock> read_frame(
        Io& io,
        const std::span<char> scratch) noexcept
    {
        auto bytes = net::Stream::read_frame16(io, scratch);
        if (!bytes) {
            return fail(bytes.error());
        }
        return parse_line(scratch.first(*bytes));
    }

    template <std::size_t Axes>
    [[nodiscard]] static Result<std::span<const MotionStep<Axes>>> plan_linear(
        const GCodeBlock block,
        const std::array<float, Axes> current,
        const float steps_mm,
        const std::span<MotionStep<Axes>> out,
        const std::uint32_t ticks_step = 1U) noexcept
    {
        if (block.command != Command::linear && block.command != Command::rapid) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        MotionSegment<Axes> segment{.ticks_step = ticks_step};
        for (std::size_t axis = 0U; axis < Axes; ++axis) {
            const auto target = block.has_axis[axis] ? block.axis[axis] : current[axis];
            segment.delta[axis] = Kinematics::round_i32((target - current[axis]) * steps_mm);
        }
        return MotionPlan<Axes>::line(out, segment);
    }

private:
    template <typename T, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T, Extent> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static bool finite(const float value) noexcept
    {
        return std::isfinite(value);
    }

    [[nodiscard]] static bool fits_i32(const float value) noexcept
    {
        const auto widened = static_cast<double>(value);
        return widened >= static_cast<double>(std::numeric_limits<int>::min()) &&
            widened <= static_cast<double>(std::numeric_limits<int>::max());
    }

    [[nodiscard]] static bool fits_u32(const float value) noexcept
    {
        const auto widened = static_cast<double>(value);
        return widened >= 0.0 &&
            widened <= static_cast<double>(std::numeric_limits<std::uint32_t>::max());
    }

    [[nodiscard]] static constexpr bool is_space(const char c) noexcept
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    [[nodiscard]] static constexpr char upper(const char c) noexcept
    {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c - ('a' - 'A')) : c;
    }

    [[nodiscard]] static constexpr std::size_t axis_index(const char c) noexcept
    {
        switch (c) {
            case 'X':
                return 0U;
            case 'Y':
                return 1U;
            case 'Z':
                return 2U;
            case 'A':
                return 3U;
            case 'B':
                return 4U;
            case 'C':
                return 5U;
            default:
                return 6U;
        }
    }

    static constexpr void apply_g(GCodeBlock& out, const int code) noexcept
    {
        switch (code) {
            case 0:
                out.command = Command::rapid;
                break;
            case 1:
                out.command = Command::linear;
                break;
            case 4:
                out.command = Command::dwell;
                break;
            case 28:
                out.command = Command::home;
                break;
            default:
                out.command = Command::none;
                break;
        }
    }
};

}  // namespace arc::cnc
