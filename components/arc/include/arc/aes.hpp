#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "aes/esp_aes.h"
#include "aes/esp_aes_gcm.h"
#include "esp_err.h"
#include "psa/crypto.h"
#include "soc/soc_caps.h"

namespace arc {

struct Aes {
    static_assert(SOC_AES_SUPPORTED, "AES accelerator is not supported on this target");

    static constexpr std::size_t block_bytes = 16U;

    enum class Way : int {
        dec = ESP_AES_DECRYPT,
        enc = ESP_AES_ENCRYPT,
    };

    Aes() noexcept
    {
        esp_aes_init(&ctx_);
    }

    Aes(const Aes&) = delete;
    Aes& operator=(const Aes&) = delete;
    Aes(Aes&&) = delete;
    Aes& operator=(Aes&&) = delete;

    ~Aes()
    {
        esp_aes_free(&ctx_);
    }

    [[nodiscard]] esp_err_t set(const void* const key, const std::size_t bytes) noexcept
    {
        if (key == nullptr || !valid(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_setkey(
            &ctx_,
            static_cast<const unsigned char*>(key),
            static_cast<unsigned>(bytes * 8U)));
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] esp_err_t set(const std::span<T, Extent> key) noexcept
    {
        return set(key.data(), key.size_bytes());
    }

    [[nodiscard]] esp_err_t block(
        const Way way,
        const void* const in,
        void* const out) noexcept
    {
        if (in == nullptr || out == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_crypt_ecb(
            &ctx_,
            static_cast<int>(way),
            static_cast<const unsigned char*>(in),
            static_cast<unsigned char*>(out)));
    }

    [[nodiscard]] esp_err_t cbc(
        const Way way,
        const void* const in,
        void* const out,
        const std::size_t bytes,
        void* const iv) noexcept
    {
        if (bytes == 0U) {
            return ESP_OK;
        }
        if (in == nullptr || out == nullptr || iv == nullptr || (bytes % block_bytes) != 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_crypt_cbc(
            &ctx_,
            static_cast<int>(way),
            bytes,
            static_cast<unsigned char*>(iv),
            static_cast<const unsigned char*>(in),
            static_cast<unsigned char*>(out)));
    }

    [[nodiscard]] esp_err_t ctr(
        const void* const in,
        void* const out,
        const std::size_t bytes,
        std::size_t& offset,
        void* const nonce,
        void* const stream) noexcept
    {
        if (bytes == 0U) {
            return ESP_OK;
        }
        if (in == nullptr || out == nullptr || nonce == nullptr || stream == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_crypt_ctr(
            &ctx_,
            bytes,
            &offset,
            static_cast<unsigned char*>(nonce),
            static_cast<unsigned char*>(stream),
            static_cast<const unsigned char*>(in),
            static_cast<unsigned char*>(out)));
    }

    [[nodiscard]] esp_err_t ofb(
        const void* const in,
        void* const out,
        const std::size_t bytes,
        std::size_t& offset,
        void* const iv) noexcept
    {
        if (bytes == 0U) {
            return ESP_OK;
        }
        if (in == nullptr || out == nullptr || iv == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_crypt_ofb(
            &ctx_,
            bytes,
            &offset,
            static_cast<unsigned char*>(iv),
            static_cast<const unsigned char*>(in),
            static_cast<unsigned char*>(out)));
    }

    [[nodiscard]] esp_aes_context* native() noexcept
    {
        return &ctx_;
    }

    [[nodiscard]] static constexpr bool valid(const std::size_t bytes) noexcept
    {
        return bytes == 16U || bytes == 24U || bytes == 32U;
    }

private:
    esp_aes_context ctx_{};

    [[nodiscard]] static constexpr esp_err_t status(const int code) noexcept
    {
        if (code == 0) {
            return ESP_OK;
        }
        if (code == ERR_ESP_AES_INVALID_KEY_LENGTH || code == ERR_ESP_AES_INVALID_INPUT_LENGTH) {
            return ESP_ERR_INVALID_ARG;
        }
        return ESP_FAIL;
    }
};

struct Gcm {
    static_assert(SOC_AES_SUPPORTED, "AES-GCM accelerator is not supported on this target");

    static constexpr int cipher = 2;

    Gcm() noexcept
    {
        esp_aes_gcm_init(&ctx_);
    }

    Gcm(const Gcm&) = delete;
    Gcm& operator=(const Gcm&) = delete;
    Gcm(Gcm&&) = delete;
    Gcm& operator=(Gcm&&) = delete;

    ~Gcm()
    {
        esp_aes_gcm_free(&ctx_);
    }

    [[nodiscard]] esp_err_t set(const void* const key, const std::size_t bytes) noexcept
    {
        if (key == nullptr || !Aes::valid(bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        return status(esp_aes_gcm_setkey(
            &ctx_,
            cipher,
            static_cast<const unsigned char*>(key),
            static_cast<unsigned>(bytes * 8U)));
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] esp_err_t set(const std::span<T, Extent> key) noexcept
    {
        return set(key.data(), key.size_bytes());
    }

    [[nodiscard]] esp_err_t seal(
        const void* const in,
        void* const out,
        const std::size_t bytes,
        const void* const iv,
        const std::size_t iv_bytes,
        const void* const aad,
        const std::size_t aad_bytes,
        void* const tag,
        const std::size_t tag_bytes = 16U) noexcept
    {
        if ((in == nullptr && bytes != 0U) || (out == nullptr && bytes != 0U) ||
            (aad == nullptr && aad_bytes != 0U) || iv == nullptr || iv_bytes == 0U ||
            tag == nullptr || tag_bytes < 4U || tag_bytes > 16U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto zero = std::uint8_t{};
        auto scratch = std::uint8_t{};
        const auto* const src = bytes == 0U ? &zero : static_cast<const std::uint8_t*>(in);
        auto* const dst = bytes == 0U ? &scratch : static_cast<std::uint8_t*>(out);

        return status(esp_aes_gcm_crypt_and_tag(
            &ctx_,
            ESP_AES_ENCRYPT,
            bytes,
            static_cast<const unsigned char*>(iv),
            iv_bytes,
            static_cast<const unsigned char*>(aad),
            aad_bytes,
            src,
            dst,
            tag_bytes,
            static_cast<unsigned char*>(tag)));
    }

    [[nodiscard]] esp_err_t open(
        const void* const in,
        void* const out,
        const std::size_t bytes,
        const void* const iv,
        const std::size_t iv_bytes,
        const void* const aad,
        const std::size_t aad_bytes,
        const void* const tag,
        const std::size_t tag_bytes = 16U) noexcept
    {
        if ((in == nullptr && bytes != 0U) || (out == nullptr && bytes != 0U) ||
            (aad == nullptr && aad_bytes != 0U) || iv == nullptr || iv_bytes == 0U ||
            tag == nullptr || tag_bytes < 4U || tag_bytes > 16U) {
            return ESP_ERR_INVALID_ARG;
        }

        const auto zero = std::uint8_t{};
        auto scratch = std::uint8_t{};
        const auto* const src = bytes == 0U ? &zero : static_cast<const std::uint8_t*>(in);
        auto* const dst = bytes == 0U ? &scratch : static_cast<std::uint8_t*>(out);

        return status(esp_aes_gcm_auth_decrypt(
            &ctx_,
            bytes,
            static_cast<const unsigned char*>(iv),
            iv_bytes,
            static_cast<const unsigned char*>(aad),
            aad_bytes,
            static_cast<const unsigned char*>(tag),
            tag_bytes,
            src,
            dst));
    }

    [[nodiscard]] esp_gcm_context* native() noexcept
    {
        return &ctx_;
    }

private:
    esp_gcm_context ctx_{};

    [[nodiscard]] static constexpr esp_err_t status(const int code) noexcept
    {
        switch (code) {
            case 0:
                return ESP_OK;
            case PSA_ERROR_INVALID_ARGUMENT:
                return ESP_ERR_INVALID_ARG;
            case PSA_ERROR_INVALID_SIGNATURE:
                return ESP_ERR_INVALID_RESPONSE;
            default:
                return ESP_FAIL;
        }
    }
};

}  // namespace arc
