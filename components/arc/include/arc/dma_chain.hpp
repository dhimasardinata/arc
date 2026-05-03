#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace arc {

struct DmaDesc {
    std::uint32_t size{};
    std::uint32_t length{};
    std::uint32_t owner{};
    std::uint32_t flags{};
    void* buffer{};
    DmaDesc* next{};
};

template <std::size_t N>
struct DmaChain {
    static_assert(N > 0U, "DMA chain must contain at least one descriptor");

    constexpr DmaChain() noexcept
    {
        link_linear();
    }

    constexpr void link_linear() noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            desc[i].next = i + 1U < N ? &desc[i + 1U] : nullptr;
        }
    }

    constexpr void link_circular() noexcept
    {
        for (std::size_t i = 0; i < N; ++i) {
            desc[i].next = &desc[(i + 1U) % N];
        }
    }

    void bind(
        const std::size_t index,
        std::span<std::uint8_t> bytes,
        const bool eof = false) noexcept
    {
        if (index >= N) {
            return;
        }
        desc[index].buffer = bytes.data();
        desc[index].size = static_cast<std::uint32_t>(bytes.size());
        desc[index].length = static_cast<std::uint32_t>(bytes.size());
        desc[index].owner = 1U;
        desc[index].flags = eof ? 1U : 0U;
    }

    [[nodiscard]] constexpr DmaDesc* head() noexcept
    {
        return desc.data();
    }

    std::array<DmaDesc, N> desc{};
};

}  // namespace arc
