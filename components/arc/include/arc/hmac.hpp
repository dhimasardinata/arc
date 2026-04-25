#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "esp_hmac.h"
#include "soc/soc_caps.h"

#include "arc/result.hpp"

namespace arc {

struct Hmac {
    static_assert(SOC_HMAC_SUPPORTED, "HMAC peripheral is not supported on this target");

    static constexpr std::size_t bytes = 32U;

    using Sum = std::array<std::uint8_t, bytes>;

    [[nodiscard]] static constexpr bool key(const hmac_key_id_t id) noexcept
    {
        if (id >= HMAC_KEY0 && id <= HMAC_KEY5) {
            return true;
        }
#if defined(SOC_KEY_MANAGER_HMAC_KEY_DEPLOY) && SOC_KEY_MANAGER_HMAC_KEY_DEPLOY
        if (id == HMAC_KEY_KM) {
            return true;
        }
#endif
        return false;
    }

    [[nodiscard]] static esp_err_t sum(
        const hmac_key_id_t id,
        const void* const data,
        const std::size_t len,
        void* const out,
        const std::size_t out_len) noexcept
    {
        if (!key(id) || out == nullptr || out_len < bytes || (data == nullptr && len != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto zero = std::uint8_t{};
        const auto* const in = len == 0U ? &zero : data;
        return esp_hmac_calculate(id, in, len, static_cast<std::uint8_t*>(out));
    }

    [[nodiscard]] static Result<Sum> sum(
        const hmac_key_id_t id,
        const void* const data,
        const std::size_t len) noexcept
    {
        Sum out{};
        const auto err = sum(id, data, len, out.data(), out.size());
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] static Result<Sum> sum(
        const hmac_key_id_t id,
        const std::span<T, Extent> data) noexcept
    {
        return sum(id, data.data(), data.size_bytes());
    }

    [[nodiscard]] static esp_err_t jtag(
        const hmac_key_id_t id,
        const void* const token,
        const std::size_t token_len = bytes) noexcept
    {
        if (!key(id) || token == nullptr || token_len < bytes) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_hmac_jtag_enable(id, static_cast<const std::uint8_t*>(token));
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] static esp_err_t jtag(
        const hmac_key_id_t id,
        const std::span<T, Extent> token) noexcept
    {
        return jtag(id, token.data(), token.size_bytes());
    }

    [[nodiscard]] static esp_err_t off() noexcept
    {
        return esp_hmac_jtag_disable();
    }
};

}  // namespace arc
