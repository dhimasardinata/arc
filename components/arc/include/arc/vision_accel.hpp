#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::vision {

enum class Pixel : std::uint8_t {
    gray8,
    rgb565,
    rgb888,
    rgba8888,
    yuv420,
    yuyv422,
    jpeg,
    h264,
};

enum class Rotate : std::uint8_t {
    deg0,
    deg90,
    deg180,
    deg270,
};

struct Rect {
    std::uint32_t x{};
    std::uint32_t y{};
    std::uint32_t w{};
    std::uint32_t h{};
};

struct InFrame {
    std::span<const std::uint8_t> data{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t stride{};
    Pixel format{};
};

struct OutFrame {
    std::span<std::uint8_t> data{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t stride{};
    Pixel format{};
};

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

[[nodiscard]] constexpr Status to_status(const Status value) noexcept
{
    return value;
}

[[nodiscard]] constexpr Status to_status(const esp_err_t value) noexcept
{
    return status(value);
}

[[nodiscard]] constexpr bool mul_size(
    const std::size_t lhs,
    const std::size_t rhs,
    std::size_t& out) noexcept
{
    if (lhs != 0U && rhs > std::numeric_limits<std::size_t>::max() / lhs) {
        return false;
    }
    out = lhs * rhs;
    return true;
}

[[nodiscard]] constexpr Result<std::size_t> row_bytes(
    const std::uint32_t width,
    const Pixel format) noexcept
{
    std::size_t bytes{};
    switch (format) {
        case Pixel::gray8:
            return static_cast<std::size_t>(width);
        case Pixel::rgb565:
        case Pixel::yuyv422:
            if (!mul_size(width, 2U, bytes)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            return bytes;
        case Pixel::rgb888:
            if (!mul_size(width, 3U, bytes)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            return bytes;
        case Pixel::rgba8888:
            if (!mul_size(width, 4U, bytes)) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            return bytes;
        case Pixel::yuv420:
        case Pixel::jpeg:
        case Pixel::h264:
            return fail(ESP_ERR_INVALID_ARG);
    }
    return fail(ESP_ERR_INVALID_ARG);
}

template <typename Policy, typename Plan>
[[nodiscard]] Status submit_srm(Policy& policy, const Plan& plan) noexcept
{
    if constexpr (requires { policy.srm(plan); }) {
        return to_status(policy.srm(plan));
    } else {
        static_cast<void>(policy);
        static_cast<void>(plan);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy, typename Plan>
[[nodiscard]] Status submit_fill(Policy& policy, const Plan& plan) noexcept
{
    if constexpr (requires { policy.fill(plan); }) {
        return to_status(policy.fill(plan));
    } else {
        static_cast<void>(policy);
        static_cast<void>(plan);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy, typename Plan>
[[nodiscard]] Status submit_blend(Policy& policy, const Plan& plan) noexcept
{
    if constexpr (requires { policy.blend(plan); }) {
        return to_status(policy.blend(plan));
    } else {
        static_cast<void>(policy);
        static_cast<void>(plan);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy, typename Plan>
[[nodiscard]] Status submit_jpeg(Policy& policy, const Plan& plan) noexcept
{
    if constexpr (requires { policy.jpeg(plan); }) {
        return to_status(policy.jpeg(plan));
    } else {
        static_cast<void>(policy);
        static_cast<void>(plan);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy, typename Plan>
[[nodiscard]] Status submit_h264(Policy& policy, const Plan& plan) noexcept
{
    if constexpr (requires { policy.h264(plan); }) {
        return to_status(policy.h264(plan));
    } else {
        static_cast<void>(policy);
        static_cast<void>(plan);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

}  // namespace detail

[[nodiscard]] constexpr bool compressed(const Pixel value) noexcept
{
    return value == Pixel::jpeg || value == Pixel::h264;
}

[[nodiscard]] constexpr bool ppa_format(const Pixel value) noexcept
{
    return !compressed(value) && value != Pixel::yuv420;
}

[[nodiscard]] constexpr Result<std::size_t> frame_bytes(
    const std::uint32_t width,
    const std::uint32_t height,
    const Pixel format,
    const std::uint32_t stride = 0U) noexcept
{
    if (width == 0U || height == 0U || compressed(format)) {
        return fail(ESP_ERR_INVALID_ARG);
    }

    if (format == Pixel::yuv420) {
        if ((width & 1U) != 0U || (height & 1U) != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto row = stride == 0U ? static_cast<std::size_t>(width) : static_cast<std::size_t>(stride);
        if (row < width || (row & 1U) != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        std::size_t luma{};
        if (!detail::mul_size(row, height, luma) ||
            luma > std::numeric_limits<std::size_t>::max() - (luma / 2U)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return luma + (luma / 2U);
    }

    const auto packed = detail::row_bytes(width, format);
    if (!packed) {
        return fail(packed.error());
    }
    const auto row = stride == 0U ? *packed : static_cast<std::size_t>(stride);
    if (row < *packed) {
        return fail(ESP_ERR_INVALID_ARG);
    }

    std::size_t total{};
    if (!detail::mul_size(row, height, total)) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return total;
}

[[nodiscard]] constexpr Rect full_frame(
    const std::uint32_t width,
    const std::uint32_t height) noexcept
{
    return Rect{.x = 0U, .y = 0U, .w = width, .h = height};
}

[[nodiscard]] constexpr bool inside(
    const Rect value,
    const std::uint32_t width,
    const std::uint32_t height) noexcept
{
    return value.w != 0U && value.h != 0U &&
        value.x <= width && value.y <= height &&
        value.w <= width - value.x && value.h <= height - value.y;
}

[[nodiscard]] constexpr Status validate(const InFrame frame) noexcept
{
    if (!detail::valid_span(frame.data)) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }
    const auto needed = frame_bytes(frame.width, frame.height, frame.format, frame.stride);
    if (!needed || frame.data.size() < *needed) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }
    return ok();
}

[[nodiscard]] constexpr Status validate(const OutFrame frame) noexcept
{
    if (!detail::valid_span(frame.data)) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }
    const auto needed = frame_bytes(frame.width, frame.height, frame.format, frame.stride);
    if (!needed || frame.data.size() < *needed) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }
    return ok();
}

struct PpaSrm {
    InFrame in{};
    Rect source{};
    OutFrame out{};
    Rect target{};
    Rotate rotate{};
    std::uint16_t sx_q8{256U};
    std::uint16_t sy_q8{256U};
    bool mirror_x{};
    bool mirror_y{};

    [[nodiscard]] Status validate() const noexcept
    {
        ARC_CHECK(::arc::vision::validate(in));
        ARC_CHECK(::arc::vision::validate(out));
        if (!ppa_format(in.format) || !ppa_format(out.format) ||
            !inside(source, in.width, in.height) || !inside(target, out.width, out.height) ||
            sx_q8 == 0U || sy_q8 == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] Status submit(Policy& policy) const noexcept
    {
        ARC_CHECK(validate());
        return detail::submit_srm(policy, *this);
    }
};

struct PpaFill {
    OutFrame out{};
    Rect target{};
    std::uint32_t argb8888{};

    [[nodiscard]] Status validate() const noexcept
    {
        ARC_CHECK(::arc::vision::validate(out));
        if (!ppa_format(out.format) || !inside(target, out.width, out.height)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] Status submit(Policy& policy) const noexcept
    {
        ARC_CHECK(validate());
        return detail::submit_fill(policy, *this);
    }
};

struct PpaBlend {
    InFrame bg{};
    InFrame fg{};
    OutFrame out{};
    Rect target{};
    std::uint8_t alpha_q8{255U};

    [[nodiscard]] Status validate() const noexcept
    {
        ARC_CHECK(::arc::vision::validate(bg));
        ARC_CHECK(::arc::vision::validate(fg));
        ARC_CHECK(::arc::vision::validate(out));
        if (!ppa_format(bg.format) || !ppa_format(fg.format) || !ppa_format(out.format) ||
            bg.width != fg.width || bg.height != fg.height ||
            bg.width != out.width || bg.height != out.height ||
            !inside(target, out.width, out.height)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] Status submit(Policy& policy) const noexcept
    {
        ARC_CHECK(validate());
        return detail::submit_blend(policy, *this);
    }
};

struct JpegEncode {
    InFrame raw{};
    std::span<std::uint8_t> out{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint8_t quality{80U};
    bool pixel_reverse{};

    [[nodiscard]] Status validate() const noexcept
    {
        ARC_CHECK(::arc::vision::validate(raw));
        if (compressed(raw.format) || !detail::valid_span(out) || out.empty() ||
            (width != 0U && raw.width != width) || (height != 0U && raw.height != height) ||
            quality == 0U || quality > 100U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] Status submit(Policy& policy) const noexcept
    {
        ARC_CHECK(validate());
        return detail::submit_jpeg(policy, *this);
    }
};

struct H264Encode {
    InFrame raw{};
    std::span<std::uint8_t> out{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t bitrate{};
    std::uint32_t fps{30U};
    bool idr{true};

    [[nodiscard]] Status validate() const noexcept
    {
        ARC_CHECK(::arc::vision::validate(raw));
        if ((raw.format != Pixel::yuv420 && raw.format != Pixel::yuyv422) ||
            (width != 0U && raw.width != width) || (height != 0U && raw.height != height) ||
            (raw.width & 1U) != 0U || (raw.height & 1U) != 0U ||
            !detail::valid_span(out) || out.empty() || bitrate == 0U || fps == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return ok();
    }

    template <typename Policy>
    [[nodiscard]] Status submit(Policy& policy) const noexcept
    {
        ARC_CHECK(validate());
        return detail::submit_h264(policy, *this);
    }
};

template <std::uint32_t Width, std::uint32_t Height, std::uint8_t Quality = 80U>
struct JpegEncoder {
    static_assert(Width != 0U && Height != 0U,
                  "[ARC ERROR] arc::vision::JpegEncoder needs a non-zero frame. Action: set Width and Height.");
    static_assert(Quality > 0U && Quality <= 100U,
                  "[ARC ERROR] arc::vision::JpegEncoder quality must be 1..100. Action: tune Quality.");

    [[nodiscard]] static constexpr JpegEncode frame(
        const InFrame raw,
        const std::span<std::uint8_t> out,
        const bool pixel_reverse = false) noexcept
    {
        return JpegEncode{
            .raw = raw,
            .out = out,
            .width = Width,
            .height = Height,
            .quality = Quality,
            .pixel_reverse = pixel_reverse,
        };
    }

    template <typename Policy>
    [[nodiscard]] static Status encode(
        Policy& policy,
        const InFrame raw,
        const std::span<std::uint8_t> out) noexcept
    {
        if (raw.width != Width || raw.height != Height) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return frame(raw, out).submit(policy);
    }
};

template <std::uint32_t Width, std::uint32_t Height, std::uint32_t Bitrate, std::uint32_t Fps = 30U>
struct H264Encoder {
    static_assert(Width != 0U && Height != 0U && (Width & 1U) == 0U && (Height & 1U) == 0U,
                  "[ARC ERROR] arc::vision::H264Encoder needs an even non-zero frame. Action: use even dimensions.");
    static_assert(Bitrate != 0U,
                  "[ARC ERROR] arc::vision::H264Encoder needs a non-zero bitrate. Action: set Bitrate.");
    static_assert(Fps != 0U,
                  "[ARC ERROR] arc::vision::H264Encoder needs a non-zero frame rate. Action: set Fps.");

    [[nodiscard]] static constexpr H264Encode frame(
        const InFrame raw,
        const std::span<std::uint8_t> out,
        const bool idr = true) noexcept
    {
        return H264Encode{
            .raw = raw,
            .out = out,
            .width = Width,
            .height = Height,
            .bitrate = Bitrate,
            .fps = Fps,
            .idr = idr,
        };
    }

    template <typename Policy>
    [[nodiscard]] static Status encode(
        Policy& policy,
        const InFrame raw,
        const std::span<std::uint8_t> out) noexcept
    {
        if (raw.width != Width || raw.height != Height) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return frame(raw, out).submit(policy);
    }
};

}  // namespace arc::vision
