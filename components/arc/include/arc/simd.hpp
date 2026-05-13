#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "arc/place.hpp"
#include "arc/result.hpp"

namespace arc {
namespace simd {

using float32x4_t = float __attribute__((vector_size(16)));
using int8x16_t = std::int8_t __attribute__((vector_size(16)));
using int16x8_t = std::int16_t __attribute__((vector_size(16)));
using uint8x16_t = std::uint8_t __attribute__((vector_size(16)));

inline constexpr std::size_t int8x16_lanes = 16U;
inline constexpr std::size_t int16x8_lanes = 8U;

template <typename V>
[[nodiscard]] inline V load_unaligned(const void* const ptr) noexcept
{
    V out{};
    std::memcpy(&out, ptr, sizeof(V));
    return out;
}

template <typename V>
inline void store_unaligned(void* const ptr, const V value) noexcept
{
    std::memcpy(ptr, &value, sizeof(V));
}

namespace pie {

struct MacS8x16 {
    static constexpr const char* instruction = "ee.mac.s8";
};

}  // namespace pie

[[nodiscard]] inline int8x16_t load_s8x16(const std::int8_t* const ptr) noexcept
{
    return load_unaligned<int8x16_t>(ptr);
}

inline void store_s8x16(std::int8_t* const ptr, const int8x16_t value) noexcept
{
    store_unaligned(ptr, value);
}

[[nodiscard]] inline int16x8_t load_s16x8(const std::int16_t* const ptr) noexcept
{
    return load_unaligned<int16x8_t>(ptr);
}

inline void store_s16x8(std::int16_t* const ptr, const int16x8_t value) noexcept
{
    store_unaligned(ptr, value);
}

[[nodiscard]] inline std::int32_t mac_s8x16(
    std::int32_t acc,
    const int8x16_t lhs,
    const int8x16_t rhs,
    const std::int32_t lhs_zero = 0,
    const std::int32_t rhs_zero = 0) noexcept
{
    for (std::size_t i = 0; i < int8x16_lanes; ++i) {
        acc += (static_cast<std::int32_t>(lhs[i]) - lhs_zero) *
            (static_cast<std::int32_t>(rhs[i]) - rhs_zero);
    }
    return acc;
}

namespace pie {

[[nodiscard]] inline std::int32_t mac_s8(
    const std::int32_t acc,
    const int8x16_t lhs,
    const int8x16_t rhs,
    const std::int32_t lhs_zero = 0,
    const std::int32_t rhs_zero = 0) noexcept
{
    return mac_s8x16(acc, lhs, rhs, lhs_zero, rhs_zero);
}

}  // namespace pie

[[nodiscard]] ARC_HOT inline std::int32_t dot_s8(
    const std::span<const std::int8_t> lhs,
    const std::span<const std::int8_t> rhs,
    const std::int32_t lhs_zero = 0,
    const std::int32_t rhs_zero = 0,
    std::int32_t acc = 0) noexcept
{
    const auto count = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
    std::size_t i{};
    for (; i + int8x16_lanes <= count; i += int8x16_lanes) {
        acc = mac_s8x16(
            acc,
            load_s8x16(lhs.data() + i),
            load_s8x16(rhs.data() + i),
            lhs_zero,
            rhs_zero);
    }
    for (; i < count; ++i) {
        acc += (static_cast<std::int32_t>(lhs[i]) - lhs_zero) *
            (static_cast<std::int32_t>(rhs[i]) - rhs_zero);
    }
    return acc;
}

[[nodiscard]] inline std::uint8_t clamp_u8(const std::int32_t value) noexcept
{
    if (value < 0) {
        return 0U;
    }
    if (value > 255) {
        return 255U;
    }
    return static_cast<std::uint8_t>(value);
}

struct Rgb565 {
    [[nodiscard]] static std::uint16_t pack(
        const std::uint8_t r,
        const std::uint8_t g,
        const std::uint8_t b) noexcept
    {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(r & 0xf8U) << 8U) |
            (static_cast<std::uint16_t>(g & 0xfcU) << 3U) |
            (static_cast<std::uint16_t>(b) >> 3U));
    }

    [[nodiscard]] static std::uint16_t from_yuv(
        const std::uint8_t y,
        const std::uint8_t u,
        const std::uint8_t v) noexcept
    {
        const auto c = static_cast<std::int32_t>(y) - 16;
        const auto d = static_cast<std::int32_t>(u) - 128;
        const auto e = static_cast<std::int32_t>(v) - 128;
        const auto r = clamp_u8((298 * c + 409 * e + 128) >> 8);
        const auto g = clamp_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
        const auto b = clamp_u8((298 * c + 516 * d + 128) >> 8);
        return pack(r, g, b);
    }

    [[nodiscard]] static Result<std::size_t> from_yuv422(
        const std::span<const std::uint8_t> yuv,
        const std::span<std::uint16_t> rgb) noexcept
    {
        if ((yuv.size() % 4U) != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto pixels = yuv.size() / 2U;
        if (rgb.size() < pixels) {
            return fail(ESP_ERR_NO_MEM);
        }

        for (std::size_t in = 0, out = 0; in < yuv.size(); in += 4U, out += 2U) {
            const auto y0 = yuv[in];
            const auto u = yuv[in + 1U];
            const auto y1 = yuv[in + 2U];
            const auto v = yuv[in + 3U];
            rgb[out] = from_yuv(y0, u, v);
            rgb[out + 1U] = from_yuv(y1, u, v);
        }
        return pixels;
    }
};

struct ComplexF32 {
    float re{};
    float im{};

    [[nodiscard]] constexpr ComplexF32 operator+(const ComplexF32 rhs) const noexcept
    {
        return {.re = re + rhs.re, .im = im + rhs.im};
    }

    [[nodiscard]] constexpr ComplexF32 operator-(const ComplexF32 rhs) const noexcept
    {
        return {.re = re - rhs.re, .im = im - rhs.im};
    }

    [[nodiscard]] constexpr ComplexF32 operator*(const ComplexF32 rhs) const noexcept
    {
        return {
            .re = (re * rhs.re) - (im * rhs.im),
            .im = (re * rhs.im) + (im * rhs.re),
        };
    }
};

[[nodiscard]] constexpr bool is_pow2(const std::size_t value) noexcept
{
    return value != 0U && (value & (value - 1U)) == 0U;
}

inline void bit_reverse(const std::span<ComplexF32> data) noexcept
{
    std::size_t j{};
    for (std::size_t i = 1; i < data.size(); ++i) {
        auto bit = data.size() >> 1U;
        for (; (j & bit) != 0U; bit >>= 1U) {
            j ^= bit;
        }
        j ^= bit;

        if (i < j) {
            const auto tmp = data[i];
            data[i] = data[j];
            data[j] = tmp;
        }
    }
}

[[nodiscard]] inline Status fft_radix2(
    const std::span<ComplexF32> data,
    const bool inverse = false) noexcept
{
    if (!is_pow2(data.size())) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    bit_reverse(data);

    constexpr auto pi = 3.14159265358979323846F;
    for (std::size_t len = 2U; len <= data.size(); len <<= 1U) {
        const auto angle = (inverse ? 2.0F : -2.0F) * pi / static_cast<float>(len);
        const ComplexF32 wlen{
            .re = static_cast<float>(std::cos(angle)),
            .im = static_cast<float>(std::sin(angle)),
        };

        for (std::size_t i = 0; i < data.size(); i += len) {
            ComplexF32 w{.re = 1.0F, .im = 0.0F};
            for (std::size_t j = 0; j < len / 2U; ++j) {
                const auto u = data[i + j];
                const auto v = data[i + j + (len / 2U)] * w;
                data[i + j] = u + v;
                data[i + j + (len / 2U)] = u - v;
                w = w * wlen;
            }
        }
    }

    if (inverse) {
        const auto scale = 1.0F / static_cast<float>(data.size());
        for (auto& value : data) {
            value.re *= scale;
            value.im *= scale;
        }
    }
    return ok();
}

}  // namespace simd
}  // namespace arc

namespace arc::simd {

[[nodiscard]] inline float32x4_t load4(const float* ptr) noexcept
{
    return load_unaligned<float32x4_t>(ptr);
}

inline void store4(float* ptr, const float32x4_t value) noexcept
{
    store_unaligned(ptr, value);
}

[[nodiscard]] inline float32x4_t splat4(const float value) noexcept
{
    const float values[4]{value, value, value, value};
    return load4(values);
}

[[nodiscard]] inline float32x4_t mac4(
    const float32x4_t acc,
    const float32x4_t lhs,
    const float32x4_t rhs) noexcept
{
    return acc + (lhs * rhs);
}

[[nodiscard]] inline float sum4(const float32x4_t value) noexcept
{
    return value[0] + value[1] + value[2] + value[3];
}

}  // namespace arc::simd
