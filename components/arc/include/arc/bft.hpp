#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::net {

template <typename T>
concept BftValue = std::is_trivially_copyable_v<T> && requires(const T& lhs, const T& rhs) {
    lhs == rhs;
};

template <typename T>
struct BftVote {
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::net::BftVote value must be trivially copyable. Action: use a flat decision type.");

    std::uint32_t node{};
    std::uint32_t view{};
    std::uint32_t seq{};
    std::uint64_t digest{};
    T value{};
};

template <typename T>
struct BftCert {
    T value{};
    std::uint64_t digest{};
    std::uint32_t view{};
    std::uint32_t seq{};
    std::uint32_t votes{};
};

template <typename T, std::size_t Peers>
struct Bft {
    static_assert(Peers >= 4U,
                  "[ARC ERROR] arc::net::Bft needs at least four peers for one Byzantine fault. Action: set Peers to 3f+1.");
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::net::Bft value must be trivially copyable. Action: use a flat decision type.");
    static_assert(BftValue<T>,
                  "[ARC ERROR] arc::net::Bft value must support ==. Action: add defaulted equality to the decision type.");

    using Vote = BftVote<T>;
    using Cert = BftCert<T>;

    std::uint32_t view{};
    std::uint32_t seq{};
    std::array<Vote, Peers> votes{};
    std::array<bool, Peers> used{};
    std::size_t seen{};

    constexpr explicit Bft(const std::uint32_t view_id = 0U, const std::uint32_t seq_id = 0U) noexcept
        : view{view_id}
        , seq{seq_id}
    {
    }

    [[nodiscard]] static consteval std::size_t faults() noexcept
    {
        return (Peers - 1U) / 3U;
    }

    [[nodiscard]] static consteval std::size_t quorum() noexcept
    {
        return (2U * faults()) + 1U;
    }

    constexpr void reset(const std::uint32_t view_id, const std::uint32_t seq_id) noexcept
    {
        view = view_id;
        seq = seq_id;
        seen = 0U;
        for (std::size_t i = 0; i < Peers; ++i) {
            votes[i] = Vote{};
            used[i] = false;
        }
    }

    [[nodiscard]] constexpr Result<Vote> vote(
        const std::uint32_t node,
        const T& value,
        const std::uint64_t digest) const noexcept
    {
        if (node == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return Vote{
            .node = node,
            .view = view,
            .seq = seq,
            .digest = digest,
            .value = value,
        };
    }

    [[nodiscard]] constexpr Status ingest(const Vote& next) noexcept
    {
        if (next.node == 0U || next.view != view || next.seq != seq) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        if (const auto existing = find(next.node); existing < Peers) {
            if (same(votes[existing], next)) {
                return ok();
            }
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }

        if (seen >= Peers) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }

        const auto slot = first_free();
        if (slot >= Peers) {
            return Status{fail(ESP_ERR_NO_MEM)};
        }
        votes[slot] = next;
        used[slot] = true;
        ++seen;
        return ok();
    }

    [[nodiscard]] constexpr Result<Cert> cert() const noexcept
    {
        for (std::size_t i = 0; i < Peers; ++i) {
            if (!used[i]) {
                continue;
            }
            const auto count = matching(votes[i]);
            if (count >= quorum()) {
                return Cert{
                    .value = votes[i].value,
                    .digest = votes[i].digest,
                    .view = view,
                    .seq = seq,
                    .votes = static_cast<std::uint32_t>(count),
                };
            }
        }
        return fail(ESP_ERR_NOT_FOUND);
    }

    [[nodiscard]] static std::span<const std::uint8_t> bytes(const Vote& vote) noexcept
    {
        return {
            reinterpret_cast<const std::uint8_t*>(&vote),
            sizeof(Vote),
        };
    }

    [[nodiscard]] static Result<Vote> parse(const std::span<const std::uint8_t> data) noexcept
    {
        if (!valid(data) || data.size() != sizeof(Vote)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        Vote out{};
        std::memcpy(&out, data.data(), sizeof(out));
        return out;
    }

private:
    [[nodiscard]] static constexpr bool same(const Vote& lhs, const Vote& rhs) noexcept
    {
        return lhs.node == rhs.node && lhs.view == rhs.view && lhs.seq == rhs.seq && lhs.digest == rhs.digest &&
            lhs.value == rhs.value;
    }

    template <typename Byte>
    [[nodiscard]] static constexpr bool valid(const std::span<Byte> data) noexcept
    {
        return data.empty() || data.data() != nullptr;
    }

    [[nodiscard]] constexpr std::size_t find(const std::uint32_t node) const noexcept
    {
        for (std::size_t i = 0; i < Peers; ++i) {
            if (used[i] && votes[i].node == node) {
                return i;
            }
        }
        return Peers;
    }

    [[nodiscard]] constexpr std::size_t first_free() const noexcept
    {
        for (std::size_t i = 0; i < Peers; ++i) {
            if (!used[i]) {
                return i;
            }
        }
        return Peers;
    }

    [[nodiscard]] constexpr std::size_t matching(const Vote& needle) const noexcept
    {
        std::size_t count{};
        for (std::size_t i = 0; i < Peers; ++i) {
            if (used[i] && votes[i].view == needle.view && votes[i].seq == needle.seq && votes[i].digest == needle.digest &&
                votes[i].value == needle.value) {
                ++count;
            }
        }
        return count;
    }
};

}  // namespace arc::net

namespace arc {

template <typename T>
using BftVote = net::BftVote<T>;

template <typename T>
using BftCert = net::BftCert<T>;

template <typename T, std::size_t Peers>
using Bft = net::Bft<T, Peers>;

}  // namespace arc
