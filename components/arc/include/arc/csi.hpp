#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

#include "arc/ml.hpp"
#include "arc/result.hpp"
#include "arc/simd.hpp"
#include "arc/spsc.hpp"

#if __has_include("esp_wifi.h")
#include "esp_wifi.h"
#endif

namespace arc::net {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool csi_valid_span(const std::span<T, Extent> data) noexcept
{
    return data.empty() || data.data() != nullptr;
}

}  // namespace detail

struct CsiConfig {
    bool lltf{true};
    bool htltf{true};
    bool stbc_htltf2{true};
    bool ltf_merge{true};
    bool channel_filter{true};
    bool manual_scale{};
    std::uint8_t shift{};
};

struct CsiMeta {
    std::array<std::uint8_t, 6> mac{};
    std::int8_t rssi{};
    std::uint8_t rate{};
    std::uint8_t channel{};
    std::uint32_t timestamp_us{};
};

struct CsiBin {
    std::int16_t i{};
    std::int16_t q{};
};

template <std::size_t Subcarriers>
struct CsiFrame {
    static_assert(Subcarriers > 0U, "CSI frame needs at least one subcarrier");
    static_assert(Subcarriers <= std::numeric_limits<std::uint16_t>::max(), "CSI frame metadata stores subcarrier count in uint16_t");

    CsiMeta meta{};
    std::array<CsiBin, Subcarriers> bins{};
    std::uint16_t used{};
};

struct CsiStats {
    float amplitude_mean{};
    float amplitude_variance{};
    float phase_delta_mean{};
    std::uint16_t bins{};
};

struct CsiRawView {
    CsiMeta meta{};
    std::span<const std::int8_t> iq{};
};

using CsiCallback = void (*)(void* user, const CsiRawView& raw);

template <typename Policy>
struct CsiRx {
    [[nodiscard]] static Status start(
        const CsiConfig config,
        const CsiCallback callback,
        void* const user = nullptr) noexcept
    {
        if (callback == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::start(config, callback, user));
    }

    [[nodiscard]] static Status stop() noexcept
    {
        return status(Policy::stop());
    }
};

struct Csi {
    template <std::size_t Subcarriers>
    [[nodiscard]] static Result<CsiFrame<Subcarriers>> frame(
        const CsiMeta meta,
        const std::span<const std::int8_t> raw_iq) noexcept
    {
        if (raw_iq.empty() || (raw_iq.size() % 2U) != 0U || !detail::csi_valid_span(raw_iq)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        const auto pairs = raw_iq.size() / 2U;
        if (pairs > Subcarriers) {
            return fail(ESP_ERR_NO_MEM);
        }

        CsiFrame<Subcarriers> out{.meta = meta, .used = static_cast<std::uint16_t>(pairs)};
        for (std::size_t i = 0; i < pairs; ++i) {
            out.bins[i] = {
                .i = raw_iq[i * 2U],
                .q = raw_iq[(i * 2U) + 1U],
            };
        }
        return out;
    }

    template <std::size_t Subcarriers, std::size_t Capacity>
    [[nodiscard]] static Status push(
        Spsc<CsiFrame<Subcarriers>, Capacity>& queue,
        const CsiMeta meta,
        const std::span<const std::int8_t> raw_iq) noexcept
    {
        auto next = frame<Subcarriers>(meta, raw_iq);
        if (!next) {
            return Status{fail(next.error())};
        }
        return queue.try_push(*next) ? ok() : Status{fail(ESP_ERR_NO_MEM)};
    }

    template <std::size_t Subcarriers>
    [[nodiscard]] static CsiStats stats(const CsiFrame<Subcarriers>& frame) noexcept
    {
        const auto used = frame.used < Subcarriers ? frame.used : Subcarriers;
        if (used == 0U) {
            return {};
        }

        float sum{};
        float sum_sq{};
        float phase_delta{};
        auto prev_phase = 0.0F;
        for (std::size_t i = 0; i < used; ++i) {
            const auto real = static_cast<float>(frame.bins[i].i);
            const auto imag = static_cast<float>(frame.bins[i].q);
            const auto amp_sq = (real * real) + (imag * imag);
            const auto amp = std::sqrt(amp_sq);
            sum += amp;
            sum_sq += amp_sq;

            const auto phase = std::atan2(imag, real);
            if (i != 0U) {
                phase_delta += unwrap(phase - prev_phase);
            }
            prev_phase = phase;
        }

        const auto count = static_cast<float>(used);
        const auto mean = sum / count;
        const auto variance = (sum_sq / count) - (mean * mean);
        return {
            .amplitude_mean = mean,
            .amplitude_variance = variance > 0.0F ? variance : 0.0F,
            .phase_delta_mean = used > 1U ? phase_delta / static_cast<float>(used - 1U) : 0.0F,
            .bins = static_cast<std::uint16_t>(used),
        };
    }

    template <std::size_t Subcarriers, std::size_t Features>
    [[nodiscard]] static Status features(
        const CsiFrame<Subcarriers>& frame,
        const std::span<float, Features> out) noexcept
    {
        if (out.size() < 3U || !detail::csi_valid_span(out)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        const auto s = stats(frame);
        out[0] = s.amplitude_mean;
        out[1] = s.amplitude_variance;
        out[2] = s.phase_delta_mean;
        for (std::size_t i = 3U; i < out.size(); ++i) {
            out[i] = 0.0F;
        }
        return ok();
    }

    template <std::size_t Features>
    [[nodiscard]] static Status quantize(
        const std::span<const float, Features> features,
        const std::span<std::int8_t, Features> out,
        const float scale,
        const std::int32_t zero = 0) noexcept
    {
        if (!std::isfinite(scale) || scale <= 0.0F ||
            (Features != 0U && (features.data() == nullptr || out.data() == nullptr))) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        for (std::size_t i = 0; i < Features; ++i) {
            const auto value = quantized_s8(features[i], scale, zero);
            if (!value) {
                return Status{fail(value.error())};
            }
            out[i] = *value;
        }
        return ok();
    }

private:
    [[nodiscard]] static Result<std::int8_t> quantized_s8(
        const float feature,
        const float scale,
        const std::int32_t zero) noexcept
    {
        if (!std::isfinite(feature)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto rounded = std::round(static_cast<double>(feature) / static_cast<double>(scale));
        const auto shifted = rounded + static_cast<double>(zero);
        if (shifted >= static_cast<double>(std::numeric_limits<std::int8_t>::max())) {
            return std::numeric_limits<std::int8_t>::max();
        }
        if (shifted <= static_cast<double>(std::numeric_limits<std::int8_t>::min())) {
            return std::numeric_limits<std::int8_t>::min();
        }
        return static_cast<std::int8_t>(shifted);
    }

    [[nodiscard]] static float unwrap(float value) noexcept
    {
        constexpr auto pi = 3.14159265358979323846F;
        while (value > pi) {
            value -= 2.0F * pi;
        }
        while (value < -pi) {
            value += 2.0F * pi;
        }
        return value;
    }
};

#if __has_include("esp_wifi.h")
struct EspWifiCsiPolicy {
    constinit static inline CsiCallback callback{};
    constinit static inline void* user{};

    [[nodiscard]] static esp_err_t start(
        const CsiConfig config,
        const CsiCallback next,
        void* const next_user) noexcept
    {
        callback = next;
        user = next_user;

        wifi_csi_config_t native{};
        native.lltf_en = config.lltf;
        native.htltf_en = config.htltf;
        native.stbc_htltf2_en = config.stbc_htltf2;
        native.ltf_merge_en = config.ltf_merge;
        native.channel_filter_en = config.channel_filter;
        native.manu_scale = config.manual_scale;
        native.shift = config.shift;
        if (const auto err = esp_wifi_set_csi_config(&native); err != ESP_OK) {
            return err;
        }
        if (const auto err = esp_wifi_set_csi_rx_cb(&on_rx, nullptr); err != ESP_OK) {
            return err;
        }
        return esp_wifi_set_csi(true);
    }

    [[nodiscard]] static esp_err_t stop() noexcept
    {
        callback = nullptr;
        user = nullptr;
        return esp_wifi_set_csi(false);
    }

private:
    static void on_rx(void*, wifi_csi_info_t* info) noexcept
    {
        if (callback == nullptr || info == nullptr || info->buf == nullptr || info->len <= 0) {
            return;
        }

        CsiRawView raw{
            .meta = {
                .rssi = static_cast<std::int8_t>(info->rx_ctrl.rssi),
                .rate = static_cast<std::uint8_t>(info->rx_ctrl.rate),
                .channel = static_cast<std::uint8_t>(info->rx_ctrl.channel),
                .timestamp_us = static_cast<std::uint32_t>(info->rx_ctrl.timestamp),
            },
            .iq = {reinterpret_cast<const std::int8_t*>(info->buf), static_cast<std::size_t>(info->len)},
        };
        std::memcpy(raw.meta.mac.data(), info->mac, raw.meta.mac.size());
        callback(user, raw);
    }
};
#endif

}  // namespace arc::net
