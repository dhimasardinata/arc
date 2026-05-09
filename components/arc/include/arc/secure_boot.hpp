#pragma once

#include <array>
#include <cstdint>

#include "arc/result.hpp"

namespace arc::secure {

struct BootState {
    bool enabled{};
    bool digest_valid{};
    std::uint8_t revoked_keys{};
};

struct BootDigest {
    std::array<std::uint8_t, 32> sha256{};
};

struct SecureBoot {
    template <typename Policy>
    [[nodiscard]] static Result<BootState> state() noexcept
    {
        return Policy::secure_boot_state();
    }

    template <typename Policy>
    [[nodiscard]] static Status revoke(const std::uint8_t key_index) noexcept
    {
        if (key_index >= 3U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::secure_boot_revoke(key_index));
    }

    template <typename Policy>
    [[nodiscard]] static Result<BootDigest> digest() noexcept
    {
        return Policy::secure_boot_digest();
    }
};

}  // namespace arc::secure
