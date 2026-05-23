#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::hls {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

enum class Interface : std::uint8_t {
    none,
    axis,
    bram,
    axi_lite,
};

struct KernelSpec {
    std::uint32_t inputs{};
    std::uint32_t outputs{};
    std::uint32_t latency_cycles{};
    Interface input_interface{Interface::none};
    Interface output_interface{Interface::none};
    bool static_bounds{true};
    bool heapless{true};
};

enum class SiliconTarget : std::uint8_t {
    hls,
    efpga,
    rtl,
};

struct SiliconPlan {
    SiliconTarget target{SiliconTarget::hls};
    std::uint32_t kernels{};
    std::uint32_t latency_cycles{};
    bool static_bounds{};
    bool heapless{};
    bool synthesizable{};
};

struct SiliconManifest {
    std::uint32_t magic{};
    SiliconTarget target{SiliconTarget::hls};
    std::uint32_t kernels{};
    std::uint32_t latency_cycles{};
    std::uint32_t io_words{};
    bool synthesizable{};
};

template <typename T>
concept StaticKernel = requires {
    { T::hls_spec() } -> std::same_as<KernelSpec>;
};

template <StaticKernel... Kernels>
[[nodiscard]] consteval std::uint64_t latency() noexcept
{
    return (std::uint64_t{} + ... + static_cast<std::uint64_t>(Kernels::hls_spec().latency_cycles));
}

template <StaticKernel... Kernels>
[[nodiscard]] consteval SiliconPlan silicon_plan(
    const SiliconTarget target = SiliconTarget::hls) noexcept
{
    static_assert(sizeof...(Kernels) > 0U,
                  "[ARC ERROR] arc::hls::silicon_plan needs at least one kernel. Action: pass fixed-shape kernel specs.");
    static_assert(sizeof...(Kernels) <= std::numeric_limits<std::uint32_t>::max(),
                  "HLS kernel count metadata is stored in uint32_t");
    static_assert(latency<Kernels...>() <= std::numeric_limits<std::uint32_t>::max(),
                  "HLS latency metadata is stored in uint32_t");
    constexpr auto bounds = (true && ... && Kernels::hls_spec().static_bounds);
    constexpr auto no_heap = (true && ... && Kernels::hls_spec().heapless);
    return {
        .target = target,
        .kernels = static_cast<std::uint32_t>(sizeof...(Kernels)),
        .latency_cycles = static_cast<std::uint32_t>(latency<Kernels...>()),
        .static_bounds = bounds,
        .heapless = no_heap,
        .synthesizable = bounds && no_heap,
    };
}

template <StaticKernel... Kernels>
[[nodiscard]] consteval std::uint32_t io_words() noexcept
{
    return (std::uint32_t{} + ... + (Kernels::hls_spec().inputs + Kernels::hls_spec().outputs));
}

template <StaticKernel... Kernels>
[[nodiscard]] consteval SiliconManifest manifest(
    const SiliconTarget target = SiliconTarget::hls) noexcept
{
    const auto plan = silicon_plan<Kernels...>(target);
    return {
        .magic = 0x41524353U,  // ARCS
        .target = plan.target,
        .kernels = plan.kernels,
        .latency_cycles = plan.latency_cycles,
        .io_words = io_words<Kernels...>(),
        .synthesizable = plan.synthesizable,
    };
}

template <std::size_t Iterations>
struct StaticLoop {
    static_assert(Iterations > 0U, "HLS static loop needs at least one iteration");

    template <typename Fn>
    static constexpr void run(Fn&& fn) noexcept(noexcept(fn(std::size_t{})))
    {
        for (std::size_t i = 0U; i < Iterations; ++i) {
            fn(i);
        }
    }
};

template <typename T, std::size_t Taps>
[[nodiscard]] constexpr Result<T> fir(
    const std::span<const T, Taps> coeffs,
    const std::span<const T, Taps> samples) noexcept
{
    static_assert(Taps > 0U, "HLS FIR needs fixed taps");
    static_assert(std::is_arithmetic_v<T>, "HLS FIR scalar must be arithmetic");
    if (!valid_span(coeffs) || !valid_span(samples)) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    T acc{};
    StaticLoop<Taps>::run([&](const std::size_t i) noexcept {
        acc += coeffs[i] * samples[i];
    });
    return acc;
}

template <typename T, std::size_t Inputs>
[[nodiscard]] constexpr Result<T> dot(
    const std::span<const T, Inputs> lhs,
    const std::span<const T, Inputs> rhs) noexcept
{
    static_assert(Inputs > 0U, "HLS dot needs fixed inputs");
    static_assert(std::is_arithmetic_v<T>, "HLS dot scalar must be arithmetic");
    if (!valid_span(lhs) || !valid_span(rhs)) {
        return fail(ESP_ERR_INVALID_ARG);
    }
    T acc{};
    StaticLoop<Inputs>::run([&](const std::size_t i) noexcept {
        acc += lhs[i] * rhs[i];
    });
    return acc;
}

template <std::size_t Inputs, std::size_t Outputs>
struct DenseSpec {
    static_assert(Inputs <= std::numeric_limits<std::uint32_t>::max(), "HLS dense input count is stored in uint32_t");
    static_assert(Outputs <= std::numeric_limits<std::uint32_t>::max(), "HLS dense output count is stored in uint32_t");
    static_assert(
        Outputs == 0U || Inputs <= std::numeric_limits<std::uint32_t>::max() / Outputs,
        "HLS dense latency metadata is stored in uint32_t");

    [[nodiscard]] static constexpr KernelSpec hls_spec() noexcept
    {
        return {
            .inputs = static_cast<std::uint32_t>(Inputs),
            .outputs = static_cast<std::uint32_t>(Outputs),
            .latency_cycles = static_cast<std::uint32_t>(Inputs * Outputs),
            .input_interface = Interface::axis,
            .output_interface = Interface::axis,
            .static_bounds = true,
            .heapless = true,
        };
    }
};

static_assert(StaticKernel<DenseSpec<1U, 1U>>);

}  // namespace arc::hls
