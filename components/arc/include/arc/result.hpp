#pragma once

#include <expected>
#include <type_traits>
#include <utility>

#include "esp_err.h"

namespace arc {

template <typename T>
using Result = std::expected<T, esp_err_t>;

using Status = std::expected<void, esp_err_t>;

[[nodiscard]] constexpr Status ok() noexcept
{
    return {};
}

[[nodiscard]] constexpr std::unexpected<esp_err_t> fail(const esp_err_t err) noexcept
{
    return std::unexpected(err);
}

template <typename T>
[[nodiscard]] constexpr Result<std::remove_cvref_t<T>> ok(T&& value) noexcept(
    std::is_nothrow_constructible_v<std::remove_cvref_t<T>, T&&>)
{
    return Result<std::remove_cvref_t<T>>{std::forward<T>(value)};
}

[[nodiscard]] constexpr Status status(const esp_err_t err) noexcept
{
    return err == ESP_OK ? ok() : Status{fail(err)};
}

}  // namespace arc
