#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc::mmu {

struct RemoteLineRequest {
    std::uint16_t node{};
    std::uintptr_t line{};
    std::uint32_t bytes{32U};
};

struct RemoteFault {
    std::uintptr_t address{};
    std::uintptr_t span_base{};
    std::size_t span_bytes{};
    std::uint16_t node{};
    std::uint8_t core{1U};
    std::uint32_t line_bytes{32U};
};

struct RemoteResume {
    RemoteLineRequest request{};
    std::uintptr_t mapped{};
    bool resumed{};
};

template <typename T>
struct DistributedSpan {
    static_assert(std::is_trivially_copyable_v<T>, "DistributedSpan elements must be trivially copyable");

    std::uintptr_t base{};
    std::size_t count{};
    std::uint16_t node{};
    std::uint32_t line_bytes{32U};

    [[nodiscard]] constexpr bool bytes_fit() const noexcept
    {
        return count <= (std::numeric_limits<std::size_t>::max() / sizeof(T));
    }

    [[nodiscard]] constexpr std::size_t bytes() const noexcept
    {
        return bytes_fit() ? count * sizeof(T) : std::numeric_limits<std::size_t>::max();
    }

    [[nodiscard]] constexpr bool range_fit() const noexcept
    {
        constexpr auto max_address = std::numeric_limits<std::uintptr_t>::max();
        const auto total = bytes();
        return bytes_fit() && total <= max_address && base <= (max_address - static_cast<std::uintptr_t>(total));
    }

    [[nodiscard]] constexpr bool contains(const std::uintptr_t address) const noexcept
    {
        return range_fit() && address >= base && (address - base) < bytes();
    }

    [[nodiscard]] constexpr Result<RemoteLineRequest> fault(const std::uintptr_t address) const noexcept
    {
        if (line_bytes == 0U || !contains(address)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto offset = address - base;
        const auto line = address - (offset % line_bytes);
        return RemoteLineRequest{.node = node, .line = line, .bytes = line_bytes};
    }
};

struct DistributedPager {
    [[nodiscard]] static constexpr Result<RemoteLineRequest> request(const RemoteFault fault) noexcept
    {
        const DistributedSpan<std::uint8_t> span{
            .base = fault.span_base,
            .count = fault.span_bytes,
            .node = fault.node,
            .line_bytes = fault.line_bytes,
        };
        return span.fault(fault.address);
    }

    template <typename Policy>
    [[nodiscard]] static Result<RemoteResume> service_fault(
        const RemoteFault fault,
        const std::span<std::uint8_t> scratch) noexcept
    {
        auto planned = request(fault);
        if (!planned) {
            return fail(planned.error());
        }
        if (scratch.size() < planned->bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto line = scratch.first(planned->bytes);
        if (const auto err = Policy::fetch_line(*planned, line); err != ESP_OK) {
            return fail(err);
        }
        if (const auto err = Policy::remap_line(fault, line); err != ESP_OK) {
            return fail(err);
        }
        if (const auto err = Policy::resume(fault.core); err != ESP_OK) {
            return fail(err);
        }

        return RemoteResume{.request = *planned, .mapped = planned->line, .resumed = true};
    }
};

}  // namespace arc::mmu
