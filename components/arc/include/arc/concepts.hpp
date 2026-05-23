#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_err.h"

#include "arc/detail/scoped_result.hpp"
#include "arc/result.hpp"

namespace arc {

template <typename T>
concept PlainPayload = detail::PlainScopedResult<std::remove_cvref_t<T>>;

template <typename T>
concept TrivialPayload = PlainPayload<T> && std::is_trivially_copyable_v<std::remove_cvref_t<T>>;

template <typename T>
concept ControlResult = std::same_as<T, void> || std::same_as<T, esp_err_t> || std::same_as<T, Status>;

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

template <typename T>
concept DigitalOut = requires {
    { T::out() } -> ControlResult;
    { T::hi() } -> ControlResult;
    { T::lo() } -> ControlResult;
    { T::toggle() } -> ControlResult;
    { T::high() } -> std::same_as<bool>;
};

template <typename T>
concept DigitalIn = requires {
    { T::in() } -> ControlResult;
    { T::high() } -> std::same_as<bool>;
};

template <typename T>
concept I2cDevice = requires(
    const void* tx,
    void* rx,
    const std::size_t tx_bytes,
    const std::size_t rx_bytes,
    const int timeout_ms) {
    { T::send(tx, tx_bytes, timeout_ms) } -> std::same_as<esp_err_t>;
    { T::recv(rx, rx_bytes, timeout_ms) } -> std::same_as<esp_err_t>;
    { T::xfer(tx, tx_bytes, rx, rx_bytes, timeout_ms) } -> std::same_as<esp_err_t>;
};

template <typename T>
concept SpiDevice = requires(
    const void* tx,
    void* rx,
    const std::size_t bytes,
    const std::uint32_t hz,
    const std::uint32_t flags) {
    { T::send(tx, bytes, hz, flags) } -> std::same_as<esp_err_t>;
    { T::recv(rx, bytes, hz, flags) } -> std::same_as<esp_err_t>;
    { T::xfer(tx, rx, bytes, hz, flags) } -> std::same_as<esp_err_t>;
};

template <typename T>
concept UartDevice = requires(
    const void* tx,
    void* rx,
    const std::size_t bytes,
    const std::uint32_t timeout_ms,
    std::size_t& available) {
    { T::write(tx, bytes) } -> std::same_as<Result<std::size_t>>;
    { T::read(rx, bytes, timeout_ms) } -> std::same_as<Result<std::size_t>>;
    { T::wait(timeout_ms) } -> std::same_as<esp_err_t>;
    { T::flush() } -> std::same_as<esp_err_t>;
    { T::available(available) } -> std::same_as<esp_err_t>;
};

}  // namespace arc
