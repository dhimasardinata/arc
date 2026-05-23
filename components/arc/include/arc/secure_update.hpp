#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

namespace arc {

struct SecureUpdateChunk {
    std::span<const std::uint8_t> ciphertext{};
    std::span<const std::uint8_t> nonce{};
    std::span<const std::uint8_t> aad{};
    std::span<const std::uint8_t> tag{};
};

template <typename Crypto, typename Writer>
struct SecureUpdate {
    [[nodiscard]] esp_err_t begin(const std::size_t image_size = 0U) noexcept
    {
        if (active_) {
            return ESP_ERR_INVALID_STATE;
        }
        const auto err = writer.begin(image_size);
        if (err == ESP_OK) {
            active_ = true;
        }
        return err;
    }

    [[nodiscard]] esp_err_t write(
        const SecureUpdateChunk& chunk,
        std::span<std::uint8_t> plaintext) noexcept
    {
        if (!valid_span(chunk.ciphertext) || !valid_span(chunk.nonce) || !valid_span(chunk.aad) ||
            !valid_span(chunk.tag) || !valid_span(plaintext)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (plaintext.size() < chunk.ciphertext.size()) {
            return ESP_ERR_NO_MEM;
        }
        if (!active_) {
            return ESP_ERR_INVALID_STATE;
        }

        const auto err = crypto.open(
            chunk.ciphertext,
            plaintext.first(chunk.ciphertext.size()),
            chunk.nonce,
            chunk.aad,
            chunk.tag);
        if (err != ESP_OK) {
            return err;
        }
        return writer.write(plaintext.data(), chunk.ciphertext.size());
    }

    [[nodiscard]] esp_err_t finish(std::span<const std::uint8_t> signature) noexcept
    {
        if (!valid_span(signature)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (!active_) {
            return ESP_ERR_INVALID_STATE;
        }

        const auto err = crypto.verify(signature);
        if (err != ESP_OK) {
            return err;
        }
        const auto done = writer.finish();
        if (done == ESP_OK) {
            active_ = false;
        }
        return done;
    }

    Crypto crypto{};
    Writer writer{};

private:
    bool active_{};

    template <typename T, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T, Extent> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }
};

}  // namespace arc
