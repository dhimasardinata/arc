#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

#include "arc/place.hpp"

namespace arc::dsp {

template <typename T, std::size_t Rows, std::size_t Cols>
struct Matrix {
    static_assert(Rows > 0U && Cols > 0U, "matrix dimensions must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "matrix scalar must be arithmetic");

    std::array<T, Rows * Cols> data{};

    [[nodiscard]] constexpr T& operator()(const std::size_t row, const std::size_t col) noexcept
    {
        return data[(row * Cols) + col];
    }

    [[nodiscard]] constexpr const T& operator()(const std::size_t row, const std::size_t col) const noexcept
    {
        return data[(row * Cols) + col];
    }
};

template <typename T, std::size_t N>
[[nodiscard]] constexpr Matrix<T, N, N> identity() noexcept
{
    Matrix<T, N, N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        out(i, i) = T{1};
    }
    return out;
}

template <typename T, std::size_t Rows, std::size_t Cols>
[[nodiscard]] constexpr Matrix<T, Rows, Cols> add(
    const Matrix<T, Rows, Cols>& lhs,
    const Matrix<T, Rows, Cols>& rhs) noexcept
{
    Matrix<T, Rows, Cols> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) {
        out.data[i] = lhs.data[i] + rhs.data[i];
    }
    return out;
}

template <typename T, std::size_t Rows, std::size_t Cols>
[[nodiscard]] constexpr Matrix<T, Rows, Cols> sub(
    const Matrix<T, Rows, Cols>& lhs,
    const Matrix<T, Rows, Cols>& rhs) noexcept
{
    Matrix<T, Rows, Cols> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) {
        out.data[i] = lhs.data[i] - rhs.data[i];
    }
    return out;
}

template <typename T, std::size_t Rows, std::size_t Cols>
[[nodiscard]] constexpr Matrix<T, Cols, Rows> transpose(const Matrix<T, Rows, Cols>& in) noexcept
{
    Matrix<T, Cols, Rows> out{};
    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t col = 0; col < Cols; ++col) {
            out(col, row) = in(row, col);
        }
    }
    return out;
}

template <typename T, std::size_t Rows, std::size_t Inner, std::size_t Cols>
[[nodiscard]] ARC_HOT inline Matrix<T, Rows, Cols> mul(
    const Matrix<T, Rows, Inner>& lhs,
    const Matrix<T, Inner, Cols>& rhs) noexcept
{
    Matrix<T, Rows, Cols> out{};
    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t col = 0; col < Cols; ++col) {
            T acc{};
            for (std::size_t k = 0; k < Inner; ++k) {
                acc += lhs(row, k) * rhs(k, col);
            }
            out(row, col) = acc;
        }
    }
    return out;
}

template <typename T, std::size_t Rows, std::size_t Cols>
[[nodiscard]] ARC_HOT inline Matrix<T, Rows, 1> mul_vec(
    const Matrix<T, Rows, Cols>& lhs,
    const Matrix<T, Cols, 1>& rhs) noexcept
{
    return mul<T, Rows, Cols, 1>(lhs, rhs);
}

}  // namespace arc::dsp
