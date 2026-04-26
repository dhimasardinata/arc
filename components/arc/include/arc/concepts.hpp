#pragma once

#include <concepts>
#include <cstdint>

#include "esp_err.h"

namespace arc {

template <typename T>
concept ControlResult = std::same_as<T, void> || std::same_as<T, esp_err_t>;

template <typename T>
concept WaveConfig = requires {
    typename T::Config;
    { T::defaults() } -> std::same_as<typename T::Config>;
    { T::config() } -> std::same_as<typename T::Config>;
    { T::permille() } -> std::same_as<std::uint32_t>;
};

template <typename T>
concept ConfigWave = WaveConfig<T> && requires(typename T::Config cfg) {
    { T::set(cfg) } -> ControlResult;
};

template <typename T>
concept DutyWave = WaveConfig<T> && requires(typename T::Config cfg, const std::uint32_t duty) {
    { T::start() } -> std::same_as<void>;
    { T::start(cfg) } -> std::same_as<void>;
    { T::duty(duty) } -> ControlResult;
};

template <typename T>
concept RateWave = WaveConfig<T> && requires(const std::uint32_t hz) {
    { T::hz() } -> std::same_as<std::uint32_t>;
    { T::hz(hz) } -> ControlResult;
};

}  // namespace arc
