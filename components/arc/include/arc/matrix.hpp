#pragma once

#include <array>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>

#include "arc/place.hpp"
#include "arc/result.hpp"

namespace arc::dsp {

template <typename T, std::size_t Rows, std::size_t Cols>
struct Matrix {
    static_assert(
        Rows > 0U && Cols > 0U,
        "[ARC ERROR] arc::dsp::Matrix dimensions must be non-zero. Action: use positive row and column counts.");
    static_assert(
        std::is_arithmetic_v<T>,
        "[ARC ERROR] arc::dsp::Matrix scalar must be arithmetic. Action: use an integer or floating-point scalar.");

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

template <std::floating_point T, std::size_t N>
[[nodiscard]] constexpr Result<Matrix<T, N, N>> inverse(Matrix<T, N, N> value) noexcept
{
    auto out = identity<T, N>();
    for (std::size_t col = 0U; col < N; ++col) {
        auto pivot = col;
        while (pivot < N && value(pivot, col) == T{}) {
            ++pivot;
        }
        if (pivot == N) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (pivot != col) {
            for (std::size_t k = 0U; k < N; ++k) {
                std::swap(value(col, k), value(pivot, k));
                std::swap(out(col, k), out(pivot, k));
            }
        }

        const auto div = value(col, col);
        for (std::size_t k = 0U; k < N; ++k) {
            value(col, k) /= div;
            out(col, k) /= div;
        }

        for (std::size_t row = 0U; row < N; ++row) {
            if (row == col) {
                continue;
            }
            const auto factor = value(row, col);
            if (factor == T{}) {
                continue;
            }
            for (std::size_t k = 0U; k < N; ++k) {
                value(row, k) -= factor * value(col, k);
                out(row, k) -= factor * out(col, k);
            }
        }
    }
    return ok(out);
}

template <std::floating_point T, std::size_t States, std::size_t Inputs>
struct Lqr {
    static_assert(
        States > 0U,
        "[ARC ERROR] arc::dsp::Lqr state count must be non-zero. Action: choose at least one plant state.");
    static_assert(
        Inputs > 0U,
        "[ARC ERROR] arc::dsp::Lqr input count must be non-zero. Action: choose at least one control input.");

    using A = Matrix<T, States, States>;
    using B = Matrix<T, States, Inputs>;
    using Q = Matrix<T, States, States>;
    using R = Matrix<T, Inputs, Inputs>;
    using P = Matrix<T, States, States>;
    using K = Matrix<T, Inputs, States>;
    using State = Matrix<T, States, 1>;
    using Input = Matrix<T, Inputs, 1>;

    [[nodiscard]] static Result<K> gain(
        const A& a,
        const B& b,
        const R& r,
        const P& p) noexcept
    {
        const auto bt = transpose(b);
        ARC_TRY(s_inv, inverse(add(r, mul(mul(bt, p), b))));
        return ok(mul(mul(mul(s_inv, bt), p), a));
    }

    [[nodiscard]] static Result<P> riccati(
        const A& a,
        const B& b,
        const Q& q,
        const R& r,
        const P& p) noexcept
    {
        ARC_TRY(k, gain(a, b, r, p));
        return ok(add(q, mul(mul(transpose(a), p), sub(a, mul(b, k)))));
    }

    [[nodiscard]] static Result<K> solve(
        const A& a,
        const B& b,
        const Q& q,
        const R& r,
        P terminal,
        const std::size_t steps) noexcept
    {
        for (std::size_t i = 0U; i < steps; ++i) {
            ARC_TRY(next, riccati(a, b, q, r, terminal));
            terminal = next;
        }
        return gain(a, b, r, terminal);
    }

    [[nodiscard]] static Input act(const K& k, const State& x) noexcept
    {
        auto out = mul_vec(k, x);
        for (auto& item : out.data) {
            item = -item;
        }
        return out;
    }
};

}  // namespace arc::dsp
