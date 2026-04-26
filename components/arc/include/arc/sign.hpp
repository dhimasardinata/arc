#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "esp_ds.h"
#include "esp_err.h"
#include "soc/soc_caps.h"

#include "arc/hmac.hpp"
#include "arc/result.hpp"

namespace arc {

struct Sign {
    static_assert(SOC_DIG_SIGN_SUPPORTED, "Digital Signature peripheral is not supported on this target");

    using Data = esp_ds_data_t;
    using Plain = esp_ds_p_data_t;
    using Length = esp_digital_signature_length_t;

    static constexpr std::size_t iv_bytes = ESP_DS_IV_LEN;
    static constexpr std::size_t key_bytes = 32U;
    static constexpr std::size_t max_bytes = ESP_DS_SIGNATURE_MAX_BIT_LEN / 8U;

    struct Job {
        Job() = default;

        Job(esp_ds_context_t* const ctx, void* const out) noexcept
            : ctx_(ctx)
            , out_(out)
        {
        }

        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;

        Job(Job&& other) noexcept
            : ctx_(std::exchange(other.ctx_, nullptr))
            , out_(std::exchange(other.out_, nullptr))
        {
        }

        Job& operator=(Job&& other) noexcept
        {
            if (this != &other) {
                close();
                ctx_ = std::exchange(other.ctx_, nullptr);
                out_ = std::exchange(other.out_, nullptr);
            }
            return *this;
        }

        ~Job()
        {
            close();
        }

        [[nodiscard]] bool busy() const noexcept
        {
            return ctx_ != nullptr && esp_ds_is_busy();
        }

        [[nodiscard]] esp_err_t finish() noexcept
        {
            if (ctx_ == nullptr || out_ == nullptr) {
                return ESP_ERR_INVALID_STATE;
            }

            auto* const ctx = std::exchange(ctx_, nullptr);
            auto* const out = std::exchange(out_, nullptr);
            return esp_ds_finish_sign(out, ctx);
        }

        [[nodiscard]] esp_ds_context_t* native() noexcept
        {
            return ctx_;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return ctx_ != nullptr;
        }

    private:
        esp_ds_context_t* ctx_{};
        void* out_{};

        void close() noexcept
        {
            if (ctx_ != nullptr && out_ != nullptr) {
                static_cast<void>(finish());
            }
        }
    };

    [[nodiscard]] static constexpr bool valid(const Length length) noexcept
    {
        return length == ESP_DS_RSA_1024 ||
            length == ESP_DS_RSA_2048 ||
            length == ESP_DS_RSA_3072 ||
            length == ESP_DS_RSA_4096;
    }

    [[nodiscard]] static constexpr std::size_t bytes(const Length length) noexcept
    {
        return valid(length) ? (static_cast<std::size_t>(length) + 1U) * 4U : 0U;
    }

    [[nodiscard]] static constexpr std::size_t bytes(const Data& data) noexcept
    {
        return bytes(data.rsa_length);
    }

    [[nodiscard]] static esp_err_t sign(
        const void* const message,
        const Data* const data,
        const hmac_key_id_t key,
        void* const signature) noexcept
    {
        if (!Hmac::key(key) || message == nullptr || data == nullptr ||
            signature == nullptr || bytes(*data) == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_ds_sign(message, data, key, signature);
    }

    [[nodiscard]] static esp_err_t sign(
        const std::span<const std::uint8_t> message,
        const Data& data,
        const hmac_key_id_t key,
        const std::span<std::uint8_t> signature) noexcept
    {
        const auto need = bytes(data);
        if (need == 0U || message.size_bytes() != need || signature.size_bytes() < need) {
            return ESP_ERR_INVALID_ARG;
        }

        return sign(message.data(), &data, key, signature.data());
    }

    [[nodiscard]] static Result<Job> start(
        const void* const message,
        const Data* const data,
        const hmac_key_id_t key,
        void* const signature) noexcept
    {
        if (!Hmac::key(key) || message == nullptr || data == nullptr ||
            signature == nullptr || bytes(*data) == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        esp_ds_context_t* ctx{};
        const auto err = esp_ds_start_sign(message, data, key, &ctx);
        if (err != ESP_OK) {
            return fail(err);
        }
        return Job{ctx, signature};
    }

    [[nodiscard]] static Result<Job> start(
        const std::span<const std::uint8_t> message,
        const Data& data,
        const hmac_key_id_t key,
        const std::span<std::uint8_t> signature) noexcept
    {
        const auto need = bytes(data);
        if (need == 0U || message.size_bytes() != need || signature.size_bytes() < need) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        return start(message.data(), &data, key, signature.data());
    }

    [[nodiscard]] static bool busy() noexcept
    {
        return esp_ds_is_busy();
    }

    [[nodiscard]] static esp_err_t seal(
        Data& out,
        const void* const iv,
        const Plain& plain,
        const void* const key) noexcept
    {
        if (iv == nullptr || key == nullptr) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_ds_encrypt_params(&out, iv, &plain, key);
    }

    [[nodiscard]] static esp_err_t seal(
        Data& out,
        const std::span<const std::uint8_t> iv,
        const Plain& plain,
        const std::span<const std::uint8_t> key) noexcept
    {
        if (iv.size_bytes() != iv_bytes || key.size_bytes() != key_bytes) {
            return ESP_ERR_INVALID_ARG;
        }

        return seal(out, iv.data(), plain, key.data());
    }
};

}  // namespace arc
