#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc {

struct MmuRegion {
    const void* address{};
    std::size_t bytes{};
    std::uintptr_t handle{};
};

struct MmuMapRequest {
    std::uintptr_t source{};
    std::size_t offset{};
    std::size_t bytes{};
    std::uint32_t memory{};
};

struct MmuSpanStubPolicy {
    [[nodiscard]] static Result<MmuRegion> map(const MmuMapRequest&) noexcept
    {
        return fail(ESP_ERR_INVALID_STATE);
    }

    static void unmap(const MmuRegion&) noexcept {}
};

template <typename T>
struct MmuSpan {
    static_assert(std::is_trivially_copyable_v<T>, "MmuSpan elements must be trivially copyable");

    MmuRegion region{};
    std::span<const T> values{};

    [[nodiscard]] const T* data() const noexcept
    {
        return values.data();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return values.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return values.empty();
    }

    [[nodiscard]] const T& operator[](const std::size_t index) const noexcept
    {
        return values[index];
    }

    template <typename Policy>
    void unmap() noexcept
    {
        Policy::unmap(region);
        region = {};
        values = {};
    }

    template <typename Policy = MmuSpanStubPolicy>
    [[nodiscard]] static Result<MmuSpan> map(const MmuMapRequest request) noexcept
    {
        if (request.bytes == 0U || (request.bytes % sizeof(T)) != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto mapped = Policy::map(request);
        if (!mapped) {
            return fail(mapped.error());
        }

        const auto* ptr = static_cast<const T*>(mapped->address);
        return ok(MmuSpan{
            .region = *mapped,
            .values = std::span<const T>{ptr, mapped->bytes / sizeof(T)},
        });
    }
};

}  // namespace arc
