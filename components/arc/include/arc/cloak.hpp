#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "arc/fence.hpp"
#include "arc/prefetch.hpp"
#include "arc/result.hpp"

namespace arc::crypto {

struct CloakPlan {
    std::uint32_t stall_mask{0x0fU};
    std::uint32_t dummy_reads{};
    std::uint32_t flatten_ticks{};
};

struct CloakStats {
    std::uint32_t stalls{};
    std::uint32_t dummy_reads{};
    std::uint32_t flatten_heavy{};
    std::uint32_t flatten_light{};
};

struct Cloak {
    template <typename Policy>
    [[nodiscard]] static CloakStats scramble(
        const CloakPlan plan,
        const std::span<const std::byte> dummy = {}) noexcept
    {
        CloakStats stats{};
        auto noise = rng<Policy>();
        const auto stalls = (noise & plan.stall_mask) + 1U;
        for (std::uint32_t i = 0U; i < stalls; ++i) {
            pause();
            ++stats.stalls;
        }
        for (std::uint32_t i = 0U; i < plan.dummy_reads && !dummy.empty(); ++i) {
            prefetch(dummy.data() + ((i + noise) % dummy.size()));
            ++stats.dummy_reads;
        }
        return stats;
    }

    template <typename Policy>
    [[nodiscard]] static CloakStats flatten(
        const CloakPlan plan,
        const std::uint32_t secret_weight) noexcept
    {
        CloakStats stats{};
        for (std::uint32_t i = 0U; i < plan.flatten_ticks; ++i) {
            const auto heavy = ((secret_weight + i) & 1U) == 0U;
            if (heavy) {
                call_heavy<Policy>();
                ++stats.flatten_heavy;
            } else {
                call_light<Policy>();
                ++stats.flatten_light;
            }
        }
        return stats;
    }

    template <typename Policy, typename Fn>
    [[nodiscard]] static decltype(auto) run(
        const CloakPlan plan,
        const std::span<const std::byte> dummy,
        Fn&& fn) noexcept(noexcept(std::forward<Fn>(fn)()))
    {
        static_cast<void>(scramble<Policy>(plan, dummy));
        decltype(auto) out = std::forward<Fn>(fn)();
        static_cast<void>(flatten<Policy>(plan, rng<Policy>()));
        return out;
    }

private:
    template <typename Policy>
    [[nodiscard]] static std::uint32_t rng() noexcept
    {
        if constexpr (requires { Policy::rng(); }) {
            return Policy::rng();
        } else {
            return 0x9e37'79b9U;
        }
    }

    template <typename Policy>
    static void call_heavy() noexcept
    {
        if constexpr (requires { Policy::heavy(); }) {
            Policy::heavy();
        } else {
            pause();
        }
    }

    template <typename Policy>
    static void call_light() noexcept
    {
        if constexpr (requires { Policy::light(); }) {
            Policy::light();
        } else {
            pause();
        }
    }
};

}  // namespace arc::crypto
