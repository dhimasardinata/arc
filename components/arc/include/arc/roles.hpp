#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

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

template <typename Result, typename... Endpoint>
inline constexpr bool scoped_role_result =
    std::is_void_v<Result> ||
    (!std::is_reference_v<Result> && !std::is_pointer_v<Result> &&
     (!std::same_as<std::remove_cvref_t<Result>, std::remove_cvref_t<Endpoint>> && ...));

template <typename Fn, typename... Args>
consteval void require_scoped_role_result()
{
    using Result = std::invoke_result_t<Fn, Args...>;
    static_assert(scoped_role_result<Result, Args...>,
                  "[ARC ERROR] scoped role callback cannot return an endpoint, reference, or pointer. Action: finish "
                  "endpoint use inside the callback and copy out an ordinary value.");
}

}  // namespace detail

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
