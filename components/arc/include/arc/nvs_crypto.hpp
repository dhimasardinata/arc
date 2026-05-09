#pragma once

#include <cstdint>
#include <string_view>

#include "arc/result.hpp"

namespace arc {

struct NvsCryptoConfig {
    std::string_view partition{"nvs"};
    std::string_view key_partition{"nvs_keys"};
    bool generate_keys{};
};

struct NvsCrypto {
    template <typename Policy>
    [[nodiscard]] static Status mount(const NvsCryptoConfig config = {}) noexcept
    {
        if (config.partition.empty() || config.key_partition.empty()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::nvs_mount_encrypted(config));
    }
};

}  // namespace arc
