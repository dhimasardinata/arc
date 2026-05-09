#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/simd.hpp"

namespace arc::dsp {

struct WavePoint3 {
    float x_mm{};
    float y_mm{};
    float z_mm{};
};

struct Transducer {
    WavePoint3 pos{};
    float gain{1.0F};
};

struct WavefrontConfig {
    std::uint32_t sample_hz{192'000U};
    std::uint32_t carrier_hz{40'000U};
    float sound_mm_s{343'000.0F};
    std::int16_t amplitude{12'000};
};

template <std::size_t Channels>
struct WavefrontPlan {
    static_assert(Channels > 0U, "wavefront needs at least one channel");

    std::array<float, Channels> phase_rad{};
    std::array<float, Channels> gain{};
};

struct Wavefront {
    template <std::size_t Channels>
    [[nodiscard]] static Result<WavefrontPlan<Channels>> focus(
        const std::span<const Transducer, Channels> array,
        const WavePoint3 target,
        const WavefrontConfig config = {}) noexcept
    {
        if (config.sample_hz == 0U || config.carrier_hz == 0U || config.sound_mm_s <= 0.0F) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        WavefrontPlan<Channels> plan{};
        constexpr auto two_pi = 6.28318530717958647692F;
        for (std::size_t i = 0U; i < Channels; ++i) {
            const auto dx = target.x_mm - array[i].pos.x_mm;
            const auto dy = target.y_mm - array[i].pos.y_mm;
            const auto dz = target.z_mm - array[i].pos.z_mm;
            const auto distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
            const auto cycles = (distance * static_cast<float>(config.carrier_hz)) / config.sound_mm_s;
            plan.phase_rad[i] = -two_pi * (cycles - std::floor(cycles));
            plan.gain[i] = array[i].gain;
        }
        return plan;
    }

    template <std::size_t Channels>
    [[nodiscard]] static Result<std::size_t> synthesize(
        const WavefrontPlan<Channels>& plan,
        const std::span<std::int16_t> interleaved,
        const std::size_t frames,
        const WavefrontConfig config = {}) noexcept
    {
        if (frames == 0U || interleaved.size() < frames * Channels ||
            config.sample_hz == 0U || config.carrier_hz == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        constexpr auto two_pi = 6.28318530717958647692F;
        const auto step = two_pi * static_cast<float>(config.carrier_hz) / static_cast<float>(config.sample_hz);
        for (std::size_t frame = 0U; frame < frames; ++frame) {
            const auto base_phase = step * static_cast<float>(frame);
            for (std::size_t channel = 0U; channel < Channels; channel += 4U) {
                simd::float32x4_t phases{};
                for (std::size_t lane = 0U; lane < 4U && channel + lane < Channels; ++lane) {
                    phases[lane] = base_phase + plan.phase_rad[channel + lane];
                }
                for (std::size_t lane = 0U; lane < 4U && channel + lane < Channels; ++lane) {
                    const auto value = std::sin(phases[lane]) * plan.gain[channel + lane] * static_cast<float>(config.amplitude);
                    interleaved[(frame * Channels) + channel + lane] = clamp_s16(value);
                }
            }
        }
        return frames * Channels;
    }

    template <typename I2sTdm, std::size_t Extent>
    [[nodiscard]] static Result<std::size_t> stream(
        const std::span<std::int16_t, Extent> interleaved,
        const std::uint32_t timeout_ms = 1000U) noexcept
    {
        return I2sTdm::write(interleaved, timeout_ms);
    }

private:
    [[nodiscard]] static constexpr std::int16_t clamp_s16(const float value) noexcept
    {
        return value > 32767.0F ? 32767 : (value < -32768.0F ? -32768 : static_cast<std::int16_t>(value));
    }
};

}  // namespace arc::dsp
