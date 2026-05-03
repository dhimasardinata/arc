#pragma once

#include <cstddef>
#include <cstdint>

namespace arc::simd {

using float32x4_t = float __attribute__((vector_size(16)));

[[nodiscard]] inline float32x4_t load4(const float* ptr) noexcept
{
    return *reinterpret_cast<const float32x4_t*>(ptr);
}

inline void store4(float* ptr, const float32x4_t value) noexcept
{
    *reinterpret_cast<float32x4_t*>(ptr) = value;
}

[[nodiscard]] inline float32x4_t splat4(const float value) noexcept
{
    return {value, value, value, value};
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
