#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dma_chain.hpp"
#include "arc/result.hpp"

namespace arc {

enum class CryptoDmaMode : std::uint8_t {
    aes_encrypt,
    aes_decrypt,
    gcm_encrypt,
    gcm_decrypt,
    sha256,
};

struct CryptoDmaJob {
    DmaDesc* input{};
    DmaDesc* output{};
    std::span<const std::uint8_t> iv{};
    std::span<const std::uint8_t> aad{};
    std::span<std::uint8_t> tag{};
    std::size_t bytes{};
    std::uint8_t key_slot{};
    CryptoDmaMode mode{CryptoDmaMode::sha256};
};

struct CryptoDmaStubPolicy {
    [[nodiscard]] static esp_err_t submit(const CryptoDmaJob&) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static bool done() noexcept
    {
        return false;
    }
};

template <typename Policy = CryptoDmaStubPolicy>
struct CryptoDma {
    [[nodiscard]] static Status submit(const CryptoDmaJob& job) noexcept
    {
        if (job.input == nullptr || job.bytes == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (job.mode != CryptoDmaMode::sha256 && job.output == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::submit(job));
    }

    [[nodiscard]] static bool done() noexcept
    {
        return Policy::done();
    }
};

template <std::size_t In, std::size_t Out, typename Policy = CryptoDmaStubPolicy>
[[nodiscard]] Status crypto_submit(
    DmaChain<In>& input,
    DmaChain<Out>& output,
    const std::size_t bytes,
    const CryptoDmaMode mode) noexcept
{
    return CryptoDma<Policy>::submit(CryptoDmaJob{
        .input = input.head(),
        .output = output.head(),
        .bytes = bytes,
        .mode = mode,
    });
}

}  // namespace arc
