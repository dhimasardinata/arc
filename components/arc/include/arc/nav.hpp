#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#include "arc/matrix.hpp"
#include "arc/result.hpp"

namespace arc::nav {

template <typename T>
struct Quaternion {
    static_assert(std::is_arithmetic_v<T>, "Quaternion scalar must be arithmetic");

    T w{1};
    T x{};
    T y{};
    T z{};

    [[nodiscard]] constexpr Quaternion operator*(const Quaternion rhs) const noexcept
    {
        return {
            .w = (w * rhs.w) - (x * rhs.x) - (y * rhs.y) - (z * rhs.z),
            .x = (w * rhs.x) + (x * rhs.w) + (y * rhs.z) - (z * rhs.y),
            .y = (w * rhs.y) - (x * rhs.z) + (y * rhs.w) + (z * rhs.x),
            .z = (w * rhs.z) + (x * rhs.y) - (y * rhs.x) + (z * rhs.w),
        };
    }

    [[nodiscard]] constexpr Quaternion blend(const Quaternion target, const T gain) const noexcept
    {
        return {
            .w = w + ((target.w - w) * gain),
            .x = x + ((target.x - x) * gain),
            .y = y + ((target.y - y) * gain),
            .z = z + ((target.z - z) * gain),
        };
    }
};

template <typename T>
struct ImuSample {
    std::array<T, 3> gyro_rad_s{};
    std::array<T, 3> accel_m_s2{};
    T dt_s{};
};

template <typename T, std::size_t ErrorStates = 15U>
struct Eskf {
    static_assert(ErrorStates >= 9U, "ESKF needs attitude, velocity, and position error states");

    using Vec3 = std::array<T, 3>;
    using ErrorVec = std::array<T, ErrorStates>;
    using Covariance = dsp::Matrix<T, ErrorStates, ErrorStates>;

    struct State {
        Quaternion<T> q{};
        Vec3 position{};
        Vec3 velocity{};
        Vec3 gyro_bias{};
        Vec3 accel_bias{};
        ErrorVec error{};
        Covariance covariance{};
    };

    [[nodiscard]] static Status predict(State& state, const ImuSample<T> imu) noexcept
    {
        if (imu.dt_s <= T{}) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const auto half_dt = imu.dt_s / T{2};
        const Quaternion<T> delta{
            .w = T{1},
            .x = (imu.gyro_rad_s[0] - state.gyro_bias[0]) * half_dt,
            .y = (imu.gyro_rad_s[1] - state.gyro_bias[1]) * half_dt,
            .z = (imu.gyro_rad_s[2] - state.gyro_bias[2]) * half_dt,
        };
        state.q = state.q * delta;
        for (std::size_t i = 0U; i < 3U; ++i) {
            const auto accel = imu.accel_m_s2[i] - state.accel_bias[i];
            state.velocity[i] += accel * imu.dt_s;
            state.position[i] += state.velocity[i] * imu.dt_s;
        }
        for (std::size_t i = 0U; i < ErrorStates; ++i) {
            state.covariance(i, i) += imu.dt_s;
        }
        return ok();
    }

    [[nodiscard]] static Status correct_pose(
        State& state,
        const Vec3 position,
        const Quaternion<T> attitude,
        const T gain) noexcept
    {
        if (gain < T{} || gain > T{1}) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        for (std::size_t i = 0U; i < 3U; ++i) {
            const auto delta = position[i] - state.position[i];
            state.error[i] = delta;
            state.position[i] += delta * gain;
        }
        state.q = state.q.blend(attitude, gain);
        return ok();
    }
};

}  // namespace arc::nav
