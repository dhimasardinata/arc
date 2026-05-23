#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/place.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::power {

struct BrownoutPlan {
    std::uint16_t death_mv{};
    std::uint16_t threshold_mv{};

    [[nodiscard]] static constexpr BrownoutPlan above_death(
        const std::uint16_t physical_death_mv) noexcept
    {
        return {
            .death_mv = physical_death_mv,
            .threshold_mv = static_cast<std::uint16_t>(physical_death_mv + 100U),
        };
    }
};

struct ResumeFrame {
    std::uintptr_t pc{};
    std::uintptr_t sp{};
    std::uint32_t regs[16]{};
    std::uint32_t generation{};
    std::uint32_t bytes{};
    std::uint32_t crc{};
};

template <std::size_t Bytes>
struct IntermittentCheckpoint {
    static_assert(Bytes > 0U, "intermittent checkpoint must have storage");

    ResumeFrame frame{};
    std::array<std::byte, Bytes> image{};
};

template <std::size_t Bytes>
struct Intermittent {
    using Checkpoint = IntermittentCheckpoint<Bytes>;

    ARC_RTC_NOINIT constinit static inline Checkpoint checkpoint{};

    [[nodiscard]] static constexpr bool valid_plan(const BrownoutPlan plan) noexcept
    {
        return plan.death_mv != 0U && plan.threshold_mv == plan.death_mv + 100U;
    }

    template <typename Policy>
    [[nodiscard]] static Status arm(const BrownoutPlan plan) noexcept
    {
        if (!valid_plan(plan)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::bod_arm(plan.threshold_mv, &brownout_isr<Policy>));
    }

    [[nodiscard]] static Status save(
        const ResumeFrame frame,
        const std::span<const std::byte> stack,
        const std::span<const std::byte> rcu_state = {}) noexcept
    {
        if (!valid_span(stack) || !valid_span(rcu_state)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (stack.size() > checkpoint.image.size() || rcu_state.size() > checkpoint.image.size() - stack.size()) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }
        const auto total = stack.size() + rcu_state.size();
        if (total > std::numeric_limits<std::uint32_t>::max()) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }

        auto out = std::span<std::byte>{checkpoint.image};
        copy_fast(out.first(stack.size()), stack);
        copy_fast(out.subspan(stack.size(), rcu_state.size()), rcu_state);

        checkpoint.frame = frame;
        checkpoint.frame.bytes = static_cast<std::uint32_t>(total);
        checkpoint.frame.generation += 1U;
        checkpoint.frame.crc = crc(std::span<const std::byte>{checkpoint.image}.first(checkpoint.frame.bytes), checkpoint.frame);
        return ok();
    }

    [[nodiscard]] static bool ready() noexcept
    {
        return checkpoint.frame.bytes <= checkpoint.image.size() &&
            checkpoint.frame.bytes != 0U &&
            checkpoint.frame.crc == crc(std::span<const std::byte>{checkpoint.image}.first(checkpoint.frame.bytes), checkpoint.frame);
    }

    static void clear() noexcept
    {
        checkpoint = {};
    }

    template <typename Policy>
    [[nodiscard]] static Status resume() noexcept
    {
        if (!ready()) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        return status(Policy::restore(
            checkpoint.frame.pc,
            checkpoint.frame.sp,
            std::span<const std::byte>{checkpoint.image.data(), checkpoint.frame.bytes}));
    }

private:
    template <typename Policy>
    static void brownout_isr(void* const context) noexcept
    {
        Policy::dying_gasp(context);
    }

    static void copy_fast(
        const std::span<std::byte> dst,
        const std::span<const std::byte> src) noexcept
    {
        auto i = std::size_t{};
        for (; i + simd::int8x16_lanes <= src.size(); i += simd::int8x16_lanes) {
            const auto v = simd::load_unaligned<simd::uint8x16_t>(src.data() + i);
            simd::store_unaligned(dst.data() + i, v);
        }
        for (; i < src.size(); ++i) {
            dst[i] = src[i];
        }
    }

    [[nodiscard]] static constexpr bool valid_span(const std::span<const std::byte> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    [[nodiscard]] static std::uint32_t crc(
        const std::span<const std::byte> bytes,
        const ResumeFrame frame) noexcept
    {
        auto h = 2'166'136'261U;
        const auto mix = [&](const std::uint32_t value) noexcept {
            h ^= value;
            h *= 16'777'619U;
        };
        mix(static_cast<std::uint32_t>(frame.pc));
        mix(static_cast<std::uint32_t>(frame.sp));
        mix(frame.generation);
        mix(frame.bytes);
        for (const auto value : bytes) {
            h ^= static_cast<std::uint8_t>(value);
            h *= 16'777'619U;
        }
        return h == 0U ? 1U : h;
    }
};

}  // namespace arc::power
