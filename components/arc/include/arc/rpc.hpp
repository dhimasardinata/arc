#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_err.h"

#include "arc/spsc.hpp"

namespace arc {

template <typename Op, typename Payload>
struct RpcRequest {
    std::uint32_t serial{};
    Op op{};
    Payload payload{};
};

template <typename Payload>
struct RpcReply {
    std::uint32_t serial{};
    esp_err_t status{ESP_OK};
    Payload payload{};
};

template <
    typename Op,
    typename RequestPayload,
    typename ReplyPayload,
    std::size_t RequestCapacity,
    std::size_t ReplyCapacity = RequestCapacity>
struct RpcLane {
    using Request = RpcRequest<Op, RequestPayload>;
    using Reply = RpcReply<ReplyPayload>;

    static_assert(std::is_integral_v<Op> || std::is_enum_v<Op>, "RPC opcode must be integral or enum");
    static_assert(std::is_trivially_copyable_v<RequestPayload>, "RPC request payload must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<ReplyPayload>, "RPC reply payload must be trivially copyable");

    [[nodiscard]] bool call(
        const Op op,
        const RequestPayload& payload,
        const std::uint32_t serial) noexcept
    {
        return requests.try_push(Request{.serial = serial, .op = op, .payload = payload});
    }

    [[nodiscard]] bool recv(Request& out) noexcept
    {
        return requests.try_pop(out);
    }

    [[nodiscard]] bool reply(
        const std::uint32_t serial,
        const esp_err_t status,
        const ReplyPayload& payload) noexcept
    {
        return replies.try_push(Reply{.serial = serial, .status = status, .payload = payload});
    }

    [[nodiscard]] bool poll(Reply& out) noexcept
    {
        return replies.try_pop(out);
    }

    [[nodiscard]] bool poll_match(
        const std::uint32_t serial,
        Reply& out) noexcept
    {
        if (deferred.space() == 0U) {
            return false;
        }

        Reply next{};
        if (!replies.try_pop(next)) {
            return false;
        }
        if (next.serial != serial) {
            if (!deferred.try_push(next)) {
                return false;
            }
            return false;
        }
        out = next;
        return true;
    }

    [[nodiscard]] bool poll_deferred(Reply& out) noexcept
    {
        return deferred.try_pop(out);
    }

    Spsc<Request, RequestCapacity> requests{};
    Spsc<Reply, ReplyCapacity> replies{};
    Spsc<Reply, ReplyCapacity> deferred{};
};

}  // namespace arc
