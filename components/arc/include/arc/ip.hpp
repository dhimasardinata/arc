#pragma once

#include <cstdint>

#include "lwip/sockets.h"

namespace arc::net {

enum class IpFamily : std::uint8_t {
    any,
    v4,
    v6,
};

[[nodiscard]] constexpr int socket_family(const IpFamily family) noexcept
{
    switch (family) {
        case IpFamily::any:
            return AF_UNSPEC;
        case IpFamily::v4:
            return AF_INET;
        case IpFamily::v6:
#if defined(AF_INET6)
            return AF_INET6;
#else
            return AF_UNSPEC;
#endif
    }
    return AF_UNSPEC;
}

}  // namespace arc::net
