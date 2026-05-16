#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

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

    static_assert(
        std::is_integral_v<Op> || std::is_enum_v<Op>,
        "[ARC ERROR] arc::RpcLane opcode must be an integral or enum type. "
        "Action: use a small enum class or integer opcode that can cross the queue boundary by value.");
    static_assert(
        std::is_trivially_copyable_v<RequestPayload>,
        "[ARC ERROR] arc::RpcLane request payload must be trivially copyable. "
        "Action: use a flat request struct with integers, enums, floats, or std::array; keep owning objects outside the RPC lane.");
    static_assert(
        std::is_trivially_copyable_v<ReplyPayload>,
        "[ARC ERROR] arc::RpcLane reply payload must be trivially copyable. "
        "Action: use a flat reply struct with integers, enums, floats, or std::array; keep owning objects outside the RPC lane.");

    class Client {
    public:
        constexpr Client() noexcept = default;

        explicit constexpr Client(RpcLane& lane) noexcept
            : lane_(&lane)
        {
        }

        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

        constexpr Client(Client&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Client& operator=(Client&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] bool call(
            const Op op,
            const RequestPayload& payload,
            const std::uint32_t serial) noexcept
        {
            return lane_ != nullptr && lane_->call(op, payload, serial);
        }

        [[nodiscard]] bool poll(Reply& out) noexcept
        {
            return lane_ != nullptr && lane_->poll(out);
        }

        [[nodiscard]] bool poll_match(
            const std::uint32_t serial,
            Reply& out) noexcept
        {
            return lane_ != nullptr && lane_->poll_match(serial, out);
        }

        [[nodiscard]] bool poll_deferred(Reply& out) noexcept
        {
            return lane_ != nullptr && lane_->poll_deferred(out);
        }

    private:
        RpcLane* lane_{};
    };

    class Server {
    public:
        constexpr Server() noexcept = default;

        explicit constexpr Server(RpcLane& lane) noexcept
            : lane_(&lane)
        {
        }

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        constexpr Server(Server&& other) noexcept
            : lane_(std::exchange(other.lane_, nullptr))
        {
        }

        constexpr Server& operator=(Server&& other) noexcept
        {
            if (this != &other) {
                lane_ = std::exchange(other.lane_, nullptr);
            }
            return *this;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return lane_ != nullptr;
        }

        [[nodiscard]] bool recv(Request& out) noexcept
        {
            return lane_ != nullptr && lane_->recv(out);
        }

        [[nodiscard]] bool reply(
            const std::uint32_t serial,
            const esp_err_t status,
            const ReplyPayload& payload) noexcept
        {
            return lane_ != nullptr && lane_->reply(serial, status, payload);
        }

    private:
        RpcLane* lane_{};
    };

    [[nodiscard]] constexpr Client client() noexcept
    {
        return Client{*this};
    }

    [[nodiscard]] constexpr Server server() noexcept
    {
        return Server{*this};
    }

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
