#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::power {

struct ProfilerConfig {
    std::uint32_t sample_hz{40'000U};
    float milliamps_per_lsb{1.0F};
};

struct PowerSample {
    std::uint32_t tick{};
    std::uint32_t pc{};
    std::uint16_t milliamps{};
};

struct Profiler {
    template <typename Policy>
    [[nodiscard]] static Result<PowerSample> sample(const std::uint32_t tick = 0U) noexcept
    {
        auto current = Policy::current_milliamps();
        if (!current) {
            return fail(current.error());
        }
        auto pc = Policy::program_counter();
        if (!pc) {
            return fail(pc.error());
        }
        return PowerSample{
            .tick = tick,
            .pc = *pc,
            .milliamps = *current,
        };
    }

    [[nodiscard]] static Result<std::span<PowerSample>> interleave(
        const std::span<const std::uint16_t> current_lsb,
        const std::span<const std::uint32_t> pcs,
        const std::span<PowerSample> out,
        const ProfilerConfig config = {}) noexcept
    {
        if (config.sample_hz == 0U || !std::isfinite(config.milliamps_per_lsb) ||
            config.milliamps_per_lsb <= 0.0F || out.empty() ||
            (current_lsb.size() != 0U && current_lsb.data() == nullptr) ||
            (pcs.size() != 0U && pcs.data() == nullptr) ||
            out.data() == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto count = min(current_lsb.size(), min(pcs.size(), out.size()));
        if (count == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto tick_step = 1'000'000U / config.sample_hz;
        if (!ticks_fit(count, tick_step)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        for (std::size_t i = 0U; i < count; ++i) {
            out[i] = {
                .tick = tick_at(i, tick_step),
                .pc = pcs[i],
                .milliamps = scale_current(current_lsb[i], config.milliamps_per_lsb),
            };
        }
        return out.first(count);
    }

    [[nodiscard]] static Result<std::span<const std::uint8_t>> perfetto_counter(
        const std::span<std::uint8_t> out,
        const PowerSample sample) noexcept
    {
        std::size_t pos{};
        if (!append(out, pos, "{\"name\":\"arc.power\",\"ph\":\"C\",\"ts\":") ||
            !append_uint(out, pos, sample.tick) ||
            !append(out, pos, ",\"args\":{\"milliamps\":") ||
            !append_uint(out, pos, sample.milliamps) ||
            !append(out, pos, ",\"pc\":") ||
            !append_uint(out, pos, sample.pc) ||
            !append(out, pos, "}}")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
    }

private:
    [[nodiscard]] static constexpr std::size_t min(
        const std::size_t lhs,
        const std::size_t rhs) noexcept
    {
        return lhs < rhs ? lhs : rhs;
    }

    [[nodiscard]] static constexpr bool ticks_fit(
        const std::size_t count,
        const std::uint32_t step) noexcept
    {
        return count == 0U || step == 0U ||
            (count - 1U) <= (std::numeric_limits<std::uint32_t>::max() / step);
    }

    [[nodiscard]] static constexpr std::uint32_t tick_at(
        const std::size_t index,
        const std::uint32_t step) noexcept
    {
        return static_cast<std::uint32_t>(index) * step;
    }

    [[nodiscard]] static std::uint16_t scale_current(
        const std::uint16_t value,
        const float scale) noexcept
    {
        const auto milliamps = static_cast<double>(value) * static_cast<double>(scale);
        if (!std::isfinite(milliamps)) {
            return 65535U;
        }
        if (milliamps <= 0.0F) {
            return 0U;
        }
        if (milliamps >= static_cast<double>(std::numeric_limits<std::uint16_t>::max())) {
            return std::numeric_limits<std::uint16_t>::max();
        }
        return static_cast<std::uint16_t>(milliamps);
    }

    [[nodiscard]] static bool append(
        const std::span<std::uint8_t> out,
        std::size_t& pos,
        const char* text) noexcept
    {
        for (auto* it = text; *it != '\0'; ++it) {
            if (pos >= out.size()) {
                return false;
            }
            out[pos++] = static_cast<std::uint8_t>(*it);
        }
        return true;
    }

    [[nodiscard]] static bool append_uint(
        const std::span<std::uint8_t> out,
        std::size_t& pos,
        const std::uint32_t value) noexcept
    {
        char tmp[10]{};
        auto len = std::size_t{};
        auto remaining = value;
        do {
            tmp[len++] = static_cast<char>('0' + (remaining % 10U));
            remaining /= 10U;
        } while (remaining != 0U && len < sizeof(tmp));

        while (len != 0U) {
            if (pos >= out.size()) {
                return false;
            }
            out[pos++] = static_cast<std::uint8_t>(tmp[--len]);
        }
        return true;
    }
};

}  // namespace arc::power
