#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::x509 {

struct CertBundle {
    std::span<const std::uint8_t> der{};
    std::size_t certificates{};
};

struct Bundle {
    template <typename Policy>
    [[nodiscard]] static Status attach(const CertBundle bundle) noexcept
    {
        if (bundle.der.empty() || bundle.certificates == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::bundle_attach(bundle));
    }
};

}  // namespace arc::x509
