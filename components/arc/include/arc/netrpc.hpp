#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"

#include "arc/pack.hpp"
#include "arc/result.hpp"
#include "arc/rpc.hpp"

namespace arc::net {

template <typename Op, typename Payload>
struct NetRpcRequest {
    std::uint32_t serial{};
    Op op{};
    Payload payload{};
};

template <typename Payload>
struct NetRpcReply {
    std::uint32_t serial{};
    std::int32_t status{};
    Payload payload{};
};

template <typename Codec>
struct NetRpc {
    template <pack::Endian Order = pack::Endian::big, typename T>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> encode(
        std::span<std::uint8_t> out,
        const T& value) noexcept
    {
        return pack::serialize<Order, Codec>(out, value);
    }

    template <pack::Endian Order = pack::Endian::big, typename T>
    [[nodiscard]] static Status decode(
        std::span<const std::uint8_t> in,
        T& out) noexcept
    {
        return pack::deserialize<Order, Codec>(in, out);
    }
};

}  // namespace arc::net
