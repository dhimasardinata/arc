#pragma once

#include <expected>
#include <type_traits>
#include <utility>

#include "esp_err.h"

#define ARC_DETAIL_CAT_INNER(lhs, rhs) lhs##rhs
#define ARC_DETAIL_CAT(lhs, rhs) ARC_DETAIL_CAT_INNER(lhs, rhs)

#define ARC_DETAIL_CHECK(expr, line) ARC_DETAIL_CHECK_IMPL(expr, line)
#define ARC_DETAIL_CHECK_IMPL(expr, line)                                 \
    do {                                                                  \
        auto ARC_DETAIL_CAT(arc_check_, line) = (expr);                   \
        if (!ARC_DETAIL_CAT(arc_check_, line)) {                          \
            return ::arc::fail(ARC_DETAIL_CAT(arc_check_, line).error()); \
        }                                                                 \
    } while (false)

#define ARC_DETAIL_TRY(name, expr, line) ARC_DETAIL_TRY_IMPL(name, expr, line)
#define ARC_DETAIL_TRY_IMPL(name, expr, line)                       \
    auto ARC_DETAIL_CAT(arc_try_, line) = (expr);                   \
    if (!ARC_DETAIL_CAT(arc_try_, line)) {                          \
        return ::arc::fail(ARC_DETAIL_CAT(arc_try_, line).error()); \
    }                                                               \
    auto name = *std::move(ARC_DETAIL_CAT(arc_try_, line))

#define ARC_CHECK(expr) ARC_DETAIL_CHECK(expr, __LINE__)
#define ARC_TRY(name, expr) ARC_DETAIL_TRY(name, expr, __LINE__)

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

[[nodiscard]] constexpr esp_err_t status_code(const Status& value) noexcept
{
    return value ? ESP_OK : value.error();
}

}  // namespace arc
