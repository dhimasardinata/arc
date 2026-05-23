#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

#include "arc/concepts.hpp"

namespace arc {

template <typename Source, typename T>
concept FlowSource = requires(T& value) {
    { Source::read(value) } noexcept -> std::same_as<bool>;
};

template <typename Sink, typename T>
concept FlowSink = requires(const T& value) {
    { Sink::write(value) } noexcept -> std::same_as<bool>;
};

template <typename Lane, typename T>
concept FlowLane = requires(Lane& lane, const T& in, T& out) {
    typename Lane::value_type;
    requires std::same_as<typename Lane::value_type, T>;
    { lane.producer().try_push(in) } noexcept -> std::same_as<bool>;
    { lane.consumer().try_pop(out) } noexcept -> std::same_as<bool>;
};

struct FlowStep {
    bool queued{};
    bool wrote{};
    bool blocked{};
    bool pending{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return queued || wrote;
    }
};

namespace detail {

template <typename Source, typename Lane, typename Sink, typename = void>
struct FlowImpl {
    static_assert(
        requires { typename Source::value_type; },
        "[ARC ERROR] arc::Flow source must declare using value_type = Payload. "
        "Action: add using value_type to the source policy so the lane and sink can be checked.");
    static_assert(
        requires { typename Lane::value_type; },
        "[ARC ERROR] arc::Flow lane must declare using value_type = Payload. "
        "Action: use an Arc lane such as arc::Spsc<T, N>, arc::Mpsc<T, N>, or a custom lane with value_type.");
};

template <typename Source, typename Lane, typename Sink>
struct FlowImpl<Source, Lane, Sink, std::void_t<typename Source::value_type, typename Lane::value_type>> {
    using value_type = typename Source::value_type;

    static_assert(
        std::same_as<typename Lane::value_type, value_type>,
        "[ARC ERROR] arc::Flow source value_type must match lane value_type. "
        "Action: use the same flat payload type for Source::value_type and the queue payload.");
    static_assert(
        std::is_trivially_copyable_v<value_type>,
        "[ARC ERROR] arc::Flow payload must be trivially copyable. "
        "Action: use a flat struct, integer, enum, or std::array as the pipeline payload.");
    static_assert(
        std::is_default_constructible_v<value_type>,
        "[ARC ERROR] arc::Flow payload must be default constructible. "
        "Action: give the flat payload default member initializers or use aggregate-zeroable fields.");
    static_assert(
        std::is_copy_assignable_v<value_type>,
        "[ARC ERROR] arc::Flow payload must be copy assignable. "
        "Action: keep queued payload fields mutable and flat, or pass stable handles outside the payload.");
    static_assert(
        PlainPayload<value_type>,
        "[ARC ERROR] arc::Flow payload cannot carry borrowed storage directly. "
        "Action: flow copied values, stable IDs, or fixed arrays; keep pointers, reference wrappers, spans, string_views, and wrappers containing them outside the payload.");
    static_assert(
        FlowSource<Source, value_type>,
        "[ARC ERROR] arc::Flow source must provide static bool read(value_type&) noexcept. "
        "Action: make Source::read fill the payload and return false when no sample is ready.");
    static_assert(
        FlowLane<Lane, value_type>,
        "[ARC ERROR] arc::Flow lane must expose producer()/consumer() endpoints for the payload. "
        "Action: use arc::Spsc<T, N>, arc::Mpsc<T, N>, or another static lane with matching endpoints.");
    static_assert(
        FlowSink<Sink, value_type>,
        "[ARC ERROR] arc::Flow sink must provide static bool write(const value_type&) noexcept. "
        "Action: make Sink::write consume one payload and return false when it cannot accept it.");

    Lane lane_{};
    value_type in_value_{};
    value_type out_value_{};
    bool in_pending_{};
    bool out_pending_{};

    [[nodiscard]] bool fill() noexcept
    {
        if (!in_pending_ && !Source::read(in_value_)) {
            return false;
        }

        if (!lane_.producer().try_push(in_value_)) {
            in_pending_ = true;
            return false;
        }

        in_pending_ = false;
        return true;
    }

    [[nodiscard]] bool drain() noexcept
    {
        if (out_pending_) {
            if (!Sink::write(out_value_)) {
                return false;
            }
            out_pending_ = false;
            return true;
        }

        value_type value{};
        if (!lane_.consumer().try_pop(value)) {
            return false;
        }
        if (!Sink::write(value)) {
            out_value_ = value;
            out_pending_ = true;
            return false;
        }
        return true;
    }

    [[nodiscard]] FlowStep step() noexcept
    {
        FlowStep out{};
        out.queued = fill();
        out.wrote = drain();
        out.blocked = pending();
        out.pending = pending();
        return out;
    }

    [[nodiscard]] std::size_t drain(const std::size_t max) noexcept
    {
        std::size_t count{};
        while (count < max && drain()) {
            ++count;
        }
        return count;
    }

    [[nodiscard]] constexpr bool pending() const noexcept
    {
        return in_pending_ || out_pending_;
    }

    [[nodiscard]] constexpr bool blocked() const noexcept
    {
        return pending();
    }

    [[nodiscard]] constexpr Lane& lane() noexcept
    {
        return lane_;
    }

    [[nodiscard]] constexpr const Lane& lane() const noexcept
    {
        return lane_;
    }

    [[nodiscard]] static constexpr std::size_t cap() noexcept
        requires requires { Lane::cap(); }
    {
        return Lane::cap();
    }
};

}  // namespace detail

template <typename Source, typename Lane, typename Sink>
struct Flow : detail::FlowImpl<Source, Lane, Sink> {
};

}  // namespace arc
