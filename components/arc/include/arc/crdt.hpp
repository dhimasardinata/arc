#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::net {

template <typename State>
concept CrdtState = std::is_trivially_copyable_v<State> && requires(State lhs, const State& rhs) {
    lhs.merge(rhs);
};

template <typename T, std::size_t Peers>
struct GCounter {
    static_assert(Peers > 0U,
                  "[ARC ERROR] arc::net::GCounter needs at least one peer slot. Action: set Peers to the fixed fleet size.");
    static_assert(std::is_unsigned_v<T>,
                  "[ARC ERROR] arc::net::GCounter values must be unsigned integers. Action: use uint32_t/uint64_t cells.");

    std::array<T, Peers> cells{};

    [[nodiscard]] constexpr Status add(const std::size_t peer, const T delta = T{1}) noexcept
    {
        if (peer >= Peers) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (std::numeric_limits<T>::max() - cells[peer] < delta) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        cells[peer] = static_cast<T>(cells[peer] + delta);
        return ok();
    }

    [[nodiscard]] constexpr bool merge(const GCounter& next) noexcept
    {
        bool changed = false;
        for (std::size_t i = 0; i < Peers; ++i) {
            if (cells[i] < next.cells[i]) {
                cells[i] = next.cells[i];
                changed = true;
            }
        }
        return changed;
    }

    [[nodiscard]] constexpr T value() const noexcept
    {
        T total{};
        for (const auto cell : cells) {
            if (std::numeric_limits<T>::max() - total < cell) {
                return std::numeric_limits<T>::max();
            }
            total = static_cast<T>(total + cell);
        }
        return total;
    }

    [[nodiscard]] constexpr Result<T> exact() const noexcept
    {
        T total{};
        for (const auto cell : cells) {
            if (std::numeric_limits<T>::max() - total < cell) {
                return fail(ESP_ERR_INVALID_STATE);
            }
            total = static_cast<T>(total + cell);
        }
        return ok(total);
    }
};

template <typename T, std::size_t Peers>
struct PnCounter {
    static_assert(Peers > 0U,
                  "[ARC ERROR] arc::net::PnCounter needs at least one peer slot. Action: set Peers to the fixed fleet size.");
    static_assert(std::is_unsigned_v<T>,
                  "[ARC ERROR] arc::net::PnCounter cells must be unsigned integers. Action: use uint32_t/uint64_t cells.");

    GCounter<T, Peers> inc{};
    GCounter<T, Peers> dec{};

    [[nodiscard]] constexpr Status add(const std::size_t peer, const T delta = T{1}) noexcept
    {
        return inc.add(peer, delta);
    }

    [[nodiscard]] constexpr Status sub(const std::size_t peer, const T delta = T{1}) noexcept
    {
        return dec.add(peer, delta);
    }

    [[nodiscard]] constexpr bool merge(const PnCounter& next) noexcept
    {
        const auto inc_changed = inc.merge(next.inc);
        const auto dec_changed = dec.merge(next.dec);
        return inc_changed || dec_changed;
    }

    [[nodiscard]] constexpr std::int64_t value() const noexcept
    {
        const auto pos = to_i64(inc.value());
        const auto neg = to_i64(dec.value());
        return pos >= neg ? pos - neg : -(neg - pos);
    }

private:
    [[nodiscard]] static constexpr std::int64_t to_i64(const T value) noexcept
    {
        constexpr auto max = std::numeric_limits<std::int64_t>::max();
        if constexpr (std::numeric_limits<T>::digits > std::numeric_limits<std::int64_t>::digits) {
            if (value > static_cast<T>(max)) {
                return max;
            }
        }
        return static_cast<std::int64_t>(value);
    }
};

template <typename T, typename Clock = std::uint64_t>
struct LwwReg {
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::net::LwwReg value must be trivially copyable. Action: use a flat telemetry struct.");
    static_assert(std::is_unsigned_v<Clock>,
                  "[ARC ERROR] arc::net::LwwReg clock must be an unsigned scalar. Action: use uint64_t logical time.");

    T data{};
    Clock stamp{};
    std::uint32_t node{};
    bool seen{};

    [[nodiscard]] constexpr Status set(const T& value, const Clock clock, const std::uint32_t owner) noexcept
    {
        if (owner == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (seen && !newer(clock, owner)) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        data = value;
        stamp = clock;
        node = owner;
        seen = true;
        return ok();
    }

    [[nodiscard]] constexpr bool merge(const LwwReg& next) noexcept
    {
        if (!next.seen || (seen && !newer(next.stamp, next.node))) {
            return false;
        }
        data = next.data;
        stamp = next.stamp;
        node = next.node;
        seen = true;
        return true;
    }

    [[nodiscard]] constexpr bool ready() const noexcept
    {
        return seen;
    }

    [[nodiscard]] constexpr const T& value() const noexcept
    {
        return data;
    }

private:
    [[nodiscard]] constexpr bool newer(const Clock clock, const std::uint32_t owner) const noexcept
    {
        return clock > stamp || (clock == stamp && owner > node);
    }
};

template <typename State, std::size_t Peers>
struct Crdt {
    static_assert(Peers > 0U,
                  "[ARC ERROR] arc::net::Crdt needs at least one peer slot. Action: set Peers to the fixed fleet size.");
    static_assert(std::is_trivially_copyable_v<State>,
                  "[ARC ERROR] arc::net::Crdt state must be trivially copyable. Action: use flat fixed-size state.");
    static_assert(CrdtState<State>,
                  "[ARC ERROR] arc::net::Crdt state must expose merge(const State&). Action: use GCounter, PnCounter, LwwReg, or a custom CvRDT.");

    struct Frame {
        std::uint32_t node{};
        State state{};
    };

    std::uint32_t self{};
    State state{};
    std::array<std::uint32_t, Peers> peers{};

    constexpr explicit Crdt(const std::uint32_t node = 0U) noexcept
        : self{node}
    {
    }

    [[nodiscard]] constexpr Status assign(const std::size_t index, const std::uint32_t node) noexcept
    {
        if (index >= Peers || node == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        for (std::size_t i = 0; i < Peers; ++i) {
            if (i != index && peers[i] == node) {
                return Status{fail(ESP_ERR_INVALID_STATE)};
            }
        }
        peers[index] = node;
        return ok();
    }

    [[nodiscard]] constexpr Result<Frame> transmit() const noexcept
    {
        if (self == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return Frame{.node = self, .state = state};
    }

    [[nodiscard]] constexpr Result<std::size_t> ingest(const Frame& next) noexcept
    {
        if (next.node == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        auto* const slot = find_slot(next.node);
        if (slot == nullptr) {
            return fail(ESP_ERR_NO_MEM);
        }
        *slot = next.node;
        static_cast<void>(state.merge(next.state));
        return static_cast<std::size_t>(slot - peers.data());
    }

    [[nodiscard]] static std::span<const std::uint8_t> bytes(const Frame& frame) noexcept
    {
        return {
            reinterpret_cast<const std::uint8_t*>(&frame),
            sizeof(Frame),
        };
    }

    [[nodiscard]] static Result<Frame> parse(const std::span<const std::uint8_t> data) noexcept
    {
        if (!valid(data) || data.size() != sizeof(Frame)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        Frame out{};
        std::memcpy(&out, data.data(), sizeof(out));
        return out;
    }

private:
    template <typename Byte>
    [[nodiscard]] static constexpr bool valid(const std::span<Byte> data) noexcept
    {
        return data.empty() || data.data() != nullptr;
    }

    [[nodiscard]] constexpr std::uint32_t* find(const std::uint32_t node) noexcept
    {
        for (auto& peer : peers) {
            if (peer == node) {
                return &peer;
            }
        }
        return nullptr;
    }

    [[nodiscard]] constexpr std::uint32_t* find_slot(const std::uint32_t node) noexcept
    {
        if (auto* const slot = find(node); slot != nullptr) {
            return slot;
        }
        for (auto& peer : peers) {
            if (peer == 0U) {
                return &peer;
            }
        }
        return nullptr;
    }
};

}  // namespace arc::net

namespace arc {

template <typename T, std::size_t Peers>
using GCounter = net::GCounter<T, Peers>;

template <typename T, std::size_t Peers>
using PnCounter = net::PnCounter<T, Peers>;

template <typename T, typename Clock = std::uint64_t>
using LwwReg = net::LwwReg<T, Clock>;

template <typename State, std::size_t Peers>
using Crdt = net::Crdt<State, Peers>;

}  // namespace arc
