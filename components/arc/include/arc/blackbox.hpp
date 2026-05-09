#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include "arc/result.hpp"

namespace arc::covert {

template <std::size_t DigestBytes = 32U>
struct BlackBoxNode {
    static_assert(DigestBytes > 0U, "black box digest must be non-zero");

    std::uint32_t seq{};
    std::uint32_t payload_bytes{};
    std::array<std::uint8_t, DigestBytes> root{};
};

struct BlackBox {
    template <typename HashPolicy, std::size_t DigestBytes = 32U>
    [[nodiscard]] static Result<BlackBoxNode<DigestBytes>> seal(
        const std::uint32_t seq,
        const std::span<const std::uint8_t> payload,
        const std::span<const std::uint8_t, DigestBytes> previous_root,
        const std::span<const std::uint8_t> die_key) noexcept
    {
        if (payload.empty() || die_key.empty() || payload.size() > std::numeric_limits<std::uint32_t>::max()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        BlackBoxNode<DigestBytes> node{
            .seq = seq,
            .payload_bytes = static_cast<std::uint32_t>(payload.size()),
        };
        HashPolicy::begin();
        HashPolicy::update(std::span<const std::uint8_t>{
            reinterpret_cast<const std::uint8_t*>(&seq),
            sizeof(seq),
        });
        HashPolicy::update(die_key);
        HashPolicy::update(previous_root);
        HashPolicy::update(payload);
        if (const auto err = HashPolicy::finish(std::span<std::uint8_t, DigestBytes>{node.root}); err != ESP_OK) {
            return fail(err);
        }
        return node;
    }

    template <typename StoragePolicy, std::size_t DigestBytes>
    [[nodiscard]] static Status append(
        const std::uint32_t offset,
        const BlackBoxNode<DigestBytes>& node,
        const std::span<const std::uint8_t> payload) noexcept
    {
        if (payload.size() != node.payload_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        const auto header = std::span<const std::uint8_t>{
            reinterpret_cast<const std::uint8_t*>(&node),
            sizeof(node),
        };
        if (const auto err = StoragePolicy::write_encrypted(offset, header); err != ESP_OK) {
            return Status{fail(err)};
        }
        return status(StoragePolicy::write_encrypted(offset + sizeof(node), payload));
    }
};

}  // namespace arc::covert
