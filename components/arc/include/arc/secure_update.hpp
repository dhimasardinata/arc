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
        return writer.begin(image_size);
    }

    [[nodiscard]] esp_err_t write(
        const SecureUpdateChunk& chunk,
        std::span<std::uint8_t> plaintext) noexcept
    {
        if (plaintext.size() < chunk.ciphertext.size()) {
            return ESP_ERR_NO_MEM;
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
        const auto err = crypto.verify(signature);
        if (err != ESP_OK) {
            return err;
        }
        return writer.finish();
    }

    Crypto crypto{};
    Writer writer{};
};

}  // namespace arc
