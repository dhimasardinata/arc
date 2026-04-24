#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "psa/crypto.h"
#include "sha/sha_core.h"
#include "soc/soc_caps.h"

#include "arc/result.hpp"

namespace arc {

struct Sha {
    static_assert(SOC_SHA_SUPPORTED, "SHA accelerator is not supported on this target");

    [[nodiscard]] static constexpr std::size_t bytes(const esp_sha_type type) noexcept
    {
        switch (type) {
            case SHA1:
                return 20U;
            case SHA2_224:
                return 28U;
            case SHA2_256:
            case SHA2_512256:
                return 32U;
            case SHA2_384:
                return 48U;
            case SHA2_512:
                return 64U;
            case SHA2_512224:
                return 28U;
            default:
                return 0U;
        }
    }

    [[nodiscard]] static constexpr psa_algorithm_t alg(const esp_sha_type type) noexcept
    {
        switch (type) {
            case SHA1:
                return PSA_ALG_SHA_1;
            case SHA2_224:
                return PSA_ALG_SHA_224;
            case SHA2_256:
                return PSA_ALG_SHA_256;
            case SHA2_384:
                return PSA_ALG_SHA_384;
            case SHA2_512:
                return PSA_ALG_SHA_512;
            case SHA2_512224:
                return PSA_ALG_SHA_512_224;
            case SHA2_512256:
                return PSA_ALG_SHA_512_256;
            default:
                return PSA_ALG_NONE;
        }
    }

    template <esp_sha_type Type>
    using Sum = std::array<std::uint8_t, bytes(Type)>;

    [[nodiscard]] static esp_err_t sum(
        const esp_sha_type type,
        const void* const data,
        const std::size_t len,
        void* const out,
        const std::size_t out_len) noexcept
    {
        const auto need = bytes(type);
        if (need == 0U || out == nullptr || out_len < need || (data == nullptr && len != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto mode = alg(type);
        if (mode == PSA_ALG_NONE) {
            return ESP_ERR_NOT_SUPPORTED;
        }

        const auto zero = std::uint8_t{};
        const auto* const in = len == 0U ? &zero : static_cast<const std::uint8_t*>(data);
        auto got = std::size_t{};
        const auto err = psa_hash_compute(
            mode,
            in,
            len,
            static_cast<std::uint8_t*>(out),
            out_len,
            &got);
        if (err != PSA_SUCCESS) {
            return status(err);
        }
        if (got != need) {
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    template <esp_sha_type Type>
    [[nodiscard]] static Result<Sum<Type>> sum(const void* const data, const std::size_t len) noexcept
    {
        Sum<Type> out{};
        const auto err = sum(Type, data, len, out.data(), out.size());
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    template <esp_sha_type Type, typename T, std::size_t Extent>
    [[nodiscard]] static Result<Sum<Type>> sum(const std::span<T, Extent> data) noexcept
    {
        return sum<Type>(data.data(), data.size_bytes());
    }

    [[nodiscard]] static Result<Sum<SHA2_256>> hash(
        const void* const data,
        const std::size_t len) noexcept
    {
        return sum<SHA2_256>(data, len);
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] static Result<Sum<SHA2_256>> hash(const std::span<T, Extent> data) noexcept
    {
        return hash(data.data(), data.size_bytes());
    }

private:
    [[nodiscard]] static constexpr esp_err_t status(const psa_status_t err) noexcept
    {
        switch (err) {
            case PSA_SUCCESS:
                return ESP_OK;
            case PSA_ERROR_NOT_SUPPORTED:
                return ESP_ERR_NOT_SUPPORTED;
            case PSA_ERROR_INVALID_ARGUMENT:
                return ESP_ERR_INVALID_ARG;
            case PSA_ERROR_BUFFER_TOO_SMALL:
                return ESP_ERR_INVALID_SIZE;
            default:
                return ESP_FAIL;
        }
    }
};

}  // namespace arc
