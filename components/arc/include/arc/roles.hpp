#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "arc/detail/scoped_result.hpp"

namespace arc {

namespace detail {

template <typename Lane>
using ProducerEndpoint = decltype(std::declval<Lane&>().producer());

template <std::size_t Index, typename Lane>
using IndexedProducerEndpoint = decltype(std::declval<Lane&>().template producer<Index>());

template <typename Lane>
using ConsumerEndpoint = decltype(std::declval<Lane&>().consumer());

template <typename Lane>
using SplitEndpoints = decltype(std::declval<Lane&>().split());

template <typename Split>
using SplitProducerEndpoint = decltype(std::declval<Split&>().producer);

template <typename Split>
using SplitConsumerEndpoint = decltype(std::declval<Split&>().consumer);

template <typename Lane>
using ClientEndpoint = decltype(std::declval<Lane&>().client());

template <typename Lane>
using ServerEndpoint = decltype(std::declval<Lane&>().server());

template <typename T>
concept RoleEndpointResult = requires(const std::remove_cvref_t<T>& endpoint) {
    { static_cast<bool>(endpoint) } -> std::same_as<bool>;
} && (requires { std::remove_cvref_t<T>::cap(); } || requires { std::remove_cvref_t<T>::producers(); } || requires { &std::remove_cvref_t<T>::call; } || requires { &std::remove_cvref_t<T>::recv; });

template <typename Result, typename... Endpoint>
inline constexpr bool scoped_role_result =
    std::is_void_v<Result> ||
    (PlainScopedResult<Result> && !RoleEndpointResult<Result> &&
     (!std::same_as<std::remove_cvref_t<Result>, std::remove_cvref_t<Endpoint>> && ...));

template <typename Fn, typename... Args>
consteval void require_scoped_role_result()
{
    using Result = std::invoke_result_t<Fn, Args...>;
    static_assert(scoped_role_result<Result, Args...>,
                  "[ARC ERROR] scoped role callback cannot return an endpoint, reference, or pointer, including a "
                  "reference wrapper or non-owning view. Action: finish "
                  "endpoint use inside the callback and copy out an ordinary value.");
}

}  // namespace detail

template <typename Endpoint>
concept RoleEndpoint = detail::RoleEndpointResult<Endpoint>;

template <typename Endpoint, typename Payload>
concept PushRole = RoleEndpoint<Endpoint> && requires(Endpoint& endpoint, const Payload& payload) {
    { endpoint.try_push(payload) } -> std::convertible_to<bool>;
};

template <typename Endpoint, typename Payload>
concept PopRole = RoleEndpoint<Endpoint> && requires(Endpoint& endpoint, Payload& payload) {
    { endpoint.try_pop(payload) } -> std::convertible_to<bool>;
};

template <typename Endpoint, typename Op, typename RequestPayload, typename Reply>
concept RpcClientRole = RoleEndpoint<Endpoint> && requires(Endpoint& endpoint, Op op, const RequestPayload& payload, std::uint32_t serial, Reply& reply) {
    { endpoint.call(op, payload, serial) } -> std::convertible_to<bool>;
    { endpoint.poll(reply) } -> std::convertible_to<bool>;
    { endpoint.poll_match(serial, reply) } -> std::convertible_to<bool>;
};

template <typename Endpoint, typename Request, typename Status, typename ReplyPayload>
concept RpcServerRole = RoleEndpoint<Endpoint> && requires(Endpoint& endpoint, Request& request, std::uint32_t serial, Status status, const ReplyPayload& payload) {
    { endpoint.recv(request) } -> std::convertible_to<bool>;
    { endpoint.reply(serial, status, payload) } -> std::convertible_to<bool>;
};

template <typename Lane>
class Roles {
public:
    constexpr Roles() noexcept = default;

    Roles(const Roles&) = delete;
    Roles& operator=(const Roles&) = delete;
    Roles(Roles&&) = delete;
    Roles& operator=(Roles&&) = delete;

    [[nodiscard]] constexpr auto producer() noexcept
        requires requires(Lane& lane) { lane.producer(); }
    {
        return lane_.producer();
    }

    template <std::size_t Index>
    [[nodiscard]] constexpr auto producer() noexcept
        requires requires(Lane& lane) { lane.template producer<Index>(); }
    {
        return lane_.template producer<Index>();
    }

    [[nodiscard]] constexpr auto consumer() noexcept
        requires requires(Lane& lane) { lane.consumer(); }
    {
        return lane_.consumer();
    }

    [[nodiscard]] constexpr auto split() noexcept
        requires requires(Lane& lane) { lane.split(); }
    {
        return lane_.split();
    }

    [[nodiscard]] constexpr auto client() noexcept
        requires requires(Lane& lane) { lane.client(); }
    {
        return lane_.client();
    }

    [[nodiscard]] constexpr auto server() noexcept
        requires requires(Lane& lane) { lane.server(); }
    {
        return lane_.server();
    }

    template <typename Fn>
        requires requires(Lane& lane) { lane.producer(); } && std::invocable<Fn, detail::ProducerEndpoint<Lane>&>
    constexpr decltype(auto) with_producer(Fn&& fn) noexcept(
        noexcept(std::invoke(std::declval<Fn>(), std::declval<detail::ProducerEndpoint<Lane>&>())))
    {
        auto endpoint = lane_.producer();
        detail::require_scoped_role_result<Fn, decltype(endpoint)&>();
        return std::invoke(std::forward<Fn>(fn), endpoint);
    }

    template <std::size_t Index, typename Fn>
        requires requires(Lane& lane) { lane.template producer<Index>(); } &&
        std::invocable<Fn, detail::IndexedProducerEndpoint<Index, Lane>&>
    constexpr decltype(auto) with_producer(Fn&& fn) noexcept(
        noexcept(std::invoke(std::declval<Fn>(),
                             std::declval<detail::IndexedProducerEndpoint<Index, Lane>&>())))
    {
        auto endpoint = lane_.template producer<Index>();
        detail::require_scoped_role_result<Fn, decltype(endpoint)&>();
        return std::invoke(std::forward<Fn>(fn), endpoint);
    }

    template <typename Fn>
        requires requires(Lane& lane) { lane.consumer(); } && std::invocable<Fn, detail::ConsumerEndpoint<Lane>&>
    constexpr decltype(auto) with_consumer(Fn&& fn) noexcept(
        noexcept(std::invoke(std::declval<Fn>(), std::declval<detail::ConsumerEndpoint<Lane>&>())))
    {
        auto endpoint = lane_.consumer();
        detail::require_scoped_role_result<Fn, decltype(endpoint)&>();
        return std::invoke(std::forward<Fn>(fn), endpoint);
    }

    template <typename Fn>
        requires requires(Lane& lane) { lane.split(); } &&
        std::invocable<Fn,
                       detail::SplitProducerEndpoint<detail::SplitEndpoints<Lane>>&,
                       detail::SplitConsumerEndpoint<detail::SplitEndpoints<Lane>>&>
    constexpr decltype(auto) with_split(Fn&& fn) noexcept(
        noexcept(std::invoke(
            std::declval<Fn>(),
            std::declval<detail::SplitProducerEndpoint<detail::SplitEndpoints<Lane>>&>(),
            std::declval<detail::SplitConsumerEndpoint<detail::SplitEndpoints<Lane>>&>())))
    {
        auto endpoints = lane_.split();
        detail::require_scoped_role_result<Fn, decltype(endpoints.producer)&, decltype(endpoints.consumer)&>();
        return std::invoke(std::forward<Fn>(fn), endpoints.producer, endpoints.consumer);
    }

    template <typename Fn>
        requires requires(Lane& lane) { lane.client(); } && std::invocable<Fn, detail::ClientEndpoint<Lane>&>
    constexpr decltype(auto) with_client(Fn&& fn) noexcept(
        noexcept(std::invoke(std::declval<Fn>(), std::declval<detail::ClientEndpoint<Lane>&>())))
    {
        auto endpoint = lane_.client();
        detail::require_scoped_role_result<Fn, decltype(endpoint)&>();
        return std::invoke(std::forward<Fn>(fn), endpoint);
    }

    template <typename Fn>
        requires requires(Lane& lane) { lane.server(); } && std::invocable<Fn, detail::ServerEndpoint<Lane>&>
    constexpr decltype(auto) with_server(Fn&& fn) noexcept(
        noexcept(std::invoke(std::declval<Fn>(), std::declval<detail::ServerEndpoint<Lane>&>())))
    {
        auto endpoint = lane_.server();
        detail::require_scoped_role_result<Fn, decltype(endpoint)&>();
        return std::invoke(std::forward<Fn>(fn), endpoint);
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
        requires requires { Lane::cap(); }
    {
        return Lane::cap();
    }

    [[nodiscard]] static constexpr std::size_t producers() noexcept
        requires requires { Lane::producers(); }
    {
        return Lane::producers();
    }

    [[nodiscard]] static constexpr std::size_t bytes() noexcept
    {
        return sizeof(Roles);
    }

private:
    Lane lane_{};
};

}  // namespace arc
