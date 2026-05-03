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
        p = add(mul(mul(a, p), transpose(a)), q);
    }

    ARC_HOT void correct_diagonal(
        const H& h,
        const R& r,
        const Measure& z) noexcept
    {
        const auto hx = mul_vec(h, x);
        const auto hp = mul(h, p);
        auto s = add(mul(hp, transpose(h)), r);
        Gain k{};

        for (std::size_t measurement = 0; measurement < Measurements; ++measurement) {
            const auto denom = s(measurement, measurement);
            if (denom == T{}) {
                continue;
            }
            for (std::size_t state = 0; state < States; ++state) {
                k(state, measurement) = hp(measurement, state) / denom;
            }
        }

        for (std::size_t state = 0; state < States; ++state) {
            T delta{};
            for (std::size_t measurement = 0; measurement < Measurements; ++measurement) {
                delta += k(state, measurement) * (z(measurement, 0) - hx(measurement, 0));
            }
            x(state, 0) += delta;
        }

        p = mul(sub(identity<T, States>(), mul(k, h)), p);
    }
};

}  // namespace arc::dsp
