#pragma once

#include <array>
#include <bit>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/detail/quant.hpp"
#include "arc/mmu_span.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::ml {

[[nodiscard]] constexpr std::int8_t saturate_s8(const std::int64_t value) noexcept
{
    if (value > std::numeric_limits<std::int8_t>::max()) {
        return std::numeric_limits<std::int8_t>::max();
    }
    if (value < std::numeric_limits<std::int8_t>::min()) {
        return std::numeric_limits<std::int8_t>::min();
    }
    return static_cast<std::int8_t>(value);
}

[[nodiscard]] constexpr std::int8_t quantize_s8(
    const std::int64_t acc,
    const std::int32_t multiplier,
    const std::uint8_t shift,
    const std::int32_t output_zero) noexcept
{
    const auto scaled = detail::round_shift_s64(detail::mul_sat(acc, multiplier), shift);
    return saturate_s8(detail::add_sat(scaled, output_zero));
}

template <typename T, std::size_t Count>
[[nodiscard]] Result<std::span<const T, Count>> mapped_weights(const MmuSpan<T>& mapped) noexcept
{
    if (mapped.size() < Count) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    return std::span<const T, Count>{mapped.data(), Count};
}

template <typename Graph>
struct Core1Inference {
    using Context = typename Graph::Context;

    static void setup(void*) noexcept {}

    static void loop(void* const context) noexcept
    {
        if (context != nullptr) {
            static_cast<void>(Graph::run(*static_cast<Context*>(context)));
        }
    }
};

template <typename T, std::size_t... Dims>
struct Tensor {
    static_assert(sizeof...(Dims) > 0U, "tensor needs at least one dimension");
    static_assert(((Dims > 0U) && ...), "tensor dimensions must be non-zero");
    static_assert(std::is_arithmetic_v<T>, "tensor element must be arithmetic");

    using value_type = T;
    static constexpr auto rank = sizeof...(Dims);
    static constexpr std::array<std::size_t, rank> shape{Dims...};
    static constexpr std::size_t size = (Dims * ...);

    std::array<T, size> values{};

    [[nodiscard]] constexpr T* data() noexcept
    {
        return values.data();
    }

    [[nodiscard]] constexpr const T* data() const noexcept
    {
        return values.data();
    }

    [[nodiscard]] constexpr std::span<T, size> span() noexcept
    {
        return std::span<T, size>{values};
    }

    [[nodiscard]] constexpr std::span<const T, size> span() const noexcept
    {
        return std::span<const T, size>{values};
    }

    [[nodiscard]] constexpr T& operator[](const std::size_t index) noexcept
    {
        return values[index];
    }

    [[nodiscard]] constexpr const T& operator[](const std::size_t index) const noexcept
    {
        return values[index];
    }
};

template <typename T, std::size_t... Dims>
inline constexpr std::array<std::size_t, Tensor<T, Dims...>::rank> Tensor<T, Dims...>::shape;

template <typename T, std::size_t N>
[[nodiscard]] constexpr T dot(
    const std::span<const T, N> lhs,
    const std::span<const T, N> rhs) noexcept
{
    T acc{};
    for (std::size_t i = 0; i < N; ++i) {
        acc = static_cast<T>(acc + (lhs[i] * rhs[i]));
    }
    return acc;
}

template <std::size_t In, std::size_t Out, typename T = float>
struct Dense {
    static_assert(In > 0U && Out > 0U, "dense layer dimensions must be non-zero");

    std::span<const T, In * Out> weights{};
    std::span<const T, Out> bias{};

    [[nodiscard]] Status eval(
        const std::span<const T, In> input,
        const std::span<T, Out> output) const noexcept
    {
        if (weights.size() != In * Out || bias.size() != Out) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t row = 0; row < Out; ++row) {
            auto acc = bias[row];
            const auto base = row * In;
            for (std::size_t col = 0; col < In; ++col) {
                acc = static_cast<T>(acc + (weights[base + col] * input[col]));
            }
            output[row] = acc;
        }
        return ok();
    }
};

template <std::size_t In, std::size_t Out>
struct QuantDenseS8 {
    static_assert(In > 0U && Out > 0U, "quant dense dimensions must be non-zero");

    std::span<const std::int8_t, In * Out> weights{};
    std::span<const std::int32_t, Out> bias{};
    std::int32_t input_zero{};
    std::int32_t weight_zero{};
    std::int32_t output_zero{};
    std::int32_t multiplier{1};
    std::uint8_t shift{};

    [[nodiscard]] Status eval(
        const std::span<const std::int8_t, In> input,
        const std::span<std::int8_t, Out> output) const noexcept
    {
        if (weights.size() != In * Out || bias.size() != Out || shift >= 31U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t row = 0; row < Out; ++row) {
            const auto base = row * In;
            const auto acc = simd::dot_s8(
                std::span<const std::int8_t>{input.data(), In},
                std::span<const std::int8_t>{weights.data() + base, In},
                input_zero,
                weight_zero,
                bias[row]);
            output[row] = quantize_s8(acc, multiplier, shift, output_zero);
        }
        return ok();
    }
};

template <std::size_t InH,
          std::size_t InW,
          std::size_t InC,
          std::size_t OutC,
          std::size_t KernelH,
          std::size_t KernelW,
          std::size_t PadH = 0U,
          std::size_t PadW = 0U,
          std::size_t StrideH = 1U,
          std::size_t StrideW = 1U>
struct Conv2dS8 {
    static_assert(InH > 0U && InW > 0U && InC > 0U && OutC > 0U, "conv2d dimensions must be non-zero");
    static_assert(KernelH > 0U && KernelW > 0U, "conv2d kernel must be non-zero");
    static_assert(StrideH > 0U && StrideW > 0U, "conv2d stride must be non-zero");
    static_assert(InH + (2U * PadH) >= KernelH, "conv2d kernel taller than padded input");
    static_assert(InW + (2U * PadW) >= KernelW, "conv2d kernel wider than padded input");

    static constexpr std::size_t out_h = ((InH + (2U * PadH) - KernelH) / StrideH) + 1U;
    static constexpr std::size_t out_w = ((InW + (2U * PadW) - KernelW) / StrideW) + 1U;
    static constexpr std::size_t input_size = InH * InW * InC;
    static constexpr std::size_t output_size = out_h * out_w * OutC;
    static constexpr std::size_t weight_size = OutC * KernelH * KernelW * InC;

    std::span<const std::int8_t, weight_size> weights{};
    std::span<const std::int32_t, OutC> bias{};
    std::int32_t input_zero{};
    std::int32_t weight_zero{};
    std::int32_t output_zero{};
    std::int32_t multiplier{1};
    std::uint8_t shift{};

    [[nodiscard]] Status eval(
        const std::span<const std::int8_t, input_size> input,
        const std::span<std::int8_t, output_size> output) const noexcept
    {
        if (weights.size() != weight_size || bias.size() != OutC || shift >= 31U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t oy = 0; oy < out_h; ++oy) {
            for (std::size_t ox = 0; ox < out_w; ++ox) {
                for (std::size_t oc = 0; oc < OutC; ++oc) {
                    auto acc = bias[oc];
                    for (std::size_t ky = 0; ky < KernelH; ++ky) {
                        const auto iy_unpadded = (oy * StrideH) + ky;
                        if (iy_unpadded < PadH || iy_unpadded >= InH + PadH) {
                            continue;
                        }
                        const auto iy = iy_unpadded - PadH;
                        for (std::size_t kx = 0; kx < KernelW; ++kx) {
                            const auto ix_unpadded = (ox * StrideW) + kx;
                            if (ix_unpadded < PadW || ix_unpadded >= InW + PadW) {
                                continue;
                            }
                            const auto ix = ix_unpadded - PadW;
                            const auto in_base = ((iy * InW) + ix) * InC;
                            const auto w_base = (((oc * KernelH + ky) * KernelW) + kx) * InC;
                            acc = simd::dot_s8(
                                std::span<const std::int8_t>{input.data() + in_base, InC},
                                std::span<const std::int8_t>{weights.data() + w_base, InC},
                                input_zero,
                                weight_zero,
                                acc);
                        }
                    }
                    output[((oy * out_w + ox) * OutC) + oc] = quantize_s8(acc, multiplier, shift, output_zero);
                }
            }
        }
        return ok();
    }
};

template <std::size_t InH,
          std::size_t InW,
          std::size_t Channels,
          std::size_t KernelH,
          std::size_t KernelW,
          std::size_t DepthMultiplier = 1U,
          std::size_t PadH = 0U,
          std::size_t PadW = 0U,
          std::size_t StrideH = 1U,
          std::size_t StrideW = 1U>
struct DepthwiseConv2dS8 {
    static_assert(InH > 0U && InW > 0U && Channels > 0U, "depthwise dimensions must be non-zero");
    static_assert(KernelH > 0U && KernelW > 0U && DepthMultiplier > 0U, "depthwise kernel must be non-zero");
    static_assert(StrideH > 0U && StrideW > 0U, "depthwise stride must be non-zero");
    static_assert(InH + (2U * PadH) >= KernelH, "depthwise kernel taller than padded input");
    static_assert(InW + (2U * PadW) >= KernelW, "depthwise kernel wider than padded input");

    static constexpr std::size_t out_h = ((InH + (2U * PadH) - KernelH) / StrideH) + 1U;
    static constexpr std::size_t out_w = ((InW + (2U * PadW) - KernelW) / StrideW) + 1U;
    static constexpr std::size_t out_c = Channels * DepthMultiplier;
    static constexpr std::size_t input_size = InH * InW * Channels;
    static constexpr std::size_t output_size = out_h * out_w * out_c;
    static constexpr std::size_t weight_size = out_c * KernelH * KernelW;

    std::span<const std::int8_t, weight_size> weights{};
    std::span<const std::int32_t, out_c> bias{};
    std::int32_t input_zero{};
    std::int32_t weight_zero{};
    std::int32_t output_zero{};
    std::int32_t multiplier{1};
    std::uint8_t shift{};

    [[nodiscard]] Status eval(
        const std::span<const std::int8_t, input_size> input,
        const std::span<std::int8_t, output_size> output) const noexcept
    {
        if (weights.size() != weight_size || bias.size() != out_c || shift >= 31U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        for (std::size_t oy = 0; oy < out_h; ++oy) {
            for (std::size_t ox = 0; ox < out_w; ++ox) {
                for (std::size_t channel = 0; channel < Channels; ++channel) {
                    for (std::size_t mult = 0; mult < DepthMultiplier; ++mult) {
                        const auto oc = (channel * DepthMultiplier) + mult;
                        auto acc = bias[oc];
                        for (std::size_t ky = 0; ky < KernelH; ++ky) {
                            const auto iy_unpadded = (oy * StrideH) + ky;
                            if (iy_unpadded < PadH || iy_unpadded >= InH + PadH) {
                                continue;
                            }
                            const auto iy = iy_unpadded - PadH;
                            for (std::size_t kx = 0; kx < KernelW; ++kx) {
                                const auto ix_unpadded = (ox * StrideW) + kx;
                                if (ix_unpadded < PadW || ix_unpadded >= InW + PadW) {
                                    continue;
                                }
                                const auto ix = ix_unpadded - PadW;
                                const auto x = static_cast<std::int32_t>(input[((iy * InW + ix) * Channels) + channel]) - input_zero;
                                const auto w = static_cast<std::int32_t>(weights[((oc * KernelH + ky) * KernelW) + kx]) - weight_zero;
                                acc += x * w;
                            }
                        }
                        output[((oy * out_w + ox) * out_c) + oc] = quantize_s8(acc, multiplier, shift, output_zero);
                    }
                }
            }
        }
        return ok();
    }
};

template <typename T,
          std::size_t InH,
          std::size_t InW,
          std::size_t Channels,
          std::size_t KernelH,
          std::size_t KernelW,
          std::size_t StrideH = KernelH,
          std::size_t StrideW = KernelW>
struct MaxPool2d {
    static_assert(std::is_arithmetic_v<T>, "maxpool element must be arithmetic");
    static_assert(InH > 0U && InW > 0U && Channels > 0U, "maxpool dimensions must be non-zero");
    static_assert(KernelH > 0U && KernelW > 0U, "maxpool kernel must be non-zero");
    static_assert(StrideH > 0U && StrideW > 0U, "maxpool stride must be non-zero");
    static_assert(InH >= KernelH && InW >= KernelW, "maxpool kernel larger than input");

    static constexpr std::size_t out_h = ((InH - KernelH) / StrideH) + 1U;
    static constexpr std::size_t out_w = ((InW - KernelW) / StrideW) + 1U;
    static constexpr std::size_t input_size = InH * InW * Channels;
    static constexpr std::size_t output_size = out_h * out_w * Channels;

    [[nodiscard]] static Status eval(
        const std::span<const T, input_size> input,
        const std::span<T, output_size> output) noexcept
    {
        for (std::size_t oy = 0; oy < out_h; ++oy) {
            for (std::size_t ox = 0; ox < out_w; ++ox) {
                for (std::size_t channel = 0; channel < Channels; ++channel) {
                    auto best = std::numeric_limits<T>::lowest();
                    for (std::size_t ky = 0; ky < KernelH; ++ky) {
                        for (std::size_t kx = 0; kx < KernelW; ++kx) {
                            const auto iy = (oy * StrideH) + ky;
                            const auto ix = (ox * StrideW) + kx;
                            best = std::max(best, input[((iy * InW + ix) * Channels) + channel]);
                        }
                    }
                    output[((oy * out_w + ox) * Channels) + channel] = best;
                }
            }
        }
        return ok();
    }
};

template <typename T, std::size_t N>
void relu(const std::span<T, N> values) noexcept
{
    for (auto& value : values) {
        if (value < T{}) {
            value = T{};
        }
    }
}

template <typename T, std::size_t N>
[[nodiscard]] std::size_t argmax(const std::span<const T, N> values) noexcept
{
    static_assert(N > 0U, "argmax needs at least one value");
    auto best = std::size_t{};
    for (std::size_t i = 1; i < N; ++i) {
        if (values[i] > values[best]) {
            best = i;
        }
    }
    return best;
}

}  // namespace arc::ml
