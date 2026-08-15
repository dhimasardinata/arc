#pragma once

#include <cstddef>

#include "arc/matrix.hpp"

namespace arc::dsp {

template <typename T, std::size_t States, std::size_t Measurements>
struct Kalman {
    using State = Matrix<T, States, 1>;
    using Measure = Matrix<T, Measurements, 1>;
    using A = Matrix<T, States, States>;
    using H = Matrix<T, Measurements, States>;
    using Q = Matrix<T, States, States>;
    using R = Matrix<T, Measurements, Measurements>;
    using P = Matrix<T, States, States>;
    using Gain = Matrix<T, States, Measurements>;

    State x{};
    P p{identity<T, States>()};

    ARC_HOT void predict(
        const A& a,
        const Q& q) noexcept
    {
        x = mul_vec(a, x);
        const auto ap = mul(a, p);
        p = mul_transpose_add(ap, a, q);
    }

    ARC_HOT void correct_diagonal(
        const H& h,
        const R& r,
        const Measure& z) noexcept
    {
        const auto hx = mul_vec(h, x);
        const auto hp = mul(h, p);
        std::array<T, Measurements> inv_denom{};
        std::array<T, Measurements> w{};

#pragma GCC unroll 4
        for (std::size_t m = 0; m < Measurements; ++m) {
            T denom = r(m, m);
#pragma GCC unroll 8
            for (std::size_t k = 0; k < States; ++k) {
                denom += hp(m, k) * h(m, k);
            }
            if (denom != T{}) {
                const auto inv = T{1} / denom;
                inv_denom[m] = inv;
                w[m] = (z(m, 0) - hx(m, 0)) * inv;
            }
        }

#pragma GCC unroll 8
        for (std::size_t state = 0; state < States; ++state) {
            T delta{};
#pragma GCC unroll 4
            for (std::size_t m = 0; m < Measurements; ++m) {
                delta += hp(m, state) * w[m];
            }
            x(state, 0) += delta;
        }

        for (std::size_t i = 0; i < States; ++i) {
#pragma GCC unroll 8
            for (std::size_t j = 0; j < States; ++j) {
                T sub_val{};
#pragma GCC unroll 4
                for (std::size_t m = 0; m < Measurements; ++m) {
                    sub_val += inv_denom[m] * hp(m, i) * hp(m, j);
                }
                p(i, j) -= sub_val;
            }
        }
    }
};

}  // namespace arc::dsp
