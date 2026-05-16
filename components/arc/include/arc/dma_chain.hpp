#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>

#include "arc/caps.hpp"
#include "arc/result.hpp"

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

    [[nodiscard]] Status try_bind(
        const std::size_t index,
        std::span<std::uint8_t> bytes,
        const bool eof = false) noexcept
    {
        if (index >= N || bytes.empty() || bytes.data() == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        desc[index].buffer = bytes.data();
        desc[index].size = static_cast<std::uint32_t>(bytes.size());
        desc[index].length = static_cast<std::uint32_t>(bytes.size());
        desc[index].owner = 1U;
        desc[index].flags = eof ? 1U : 0U;
        return ok();
    }

    template <typename T, std::size_t Extent>
        requires(!std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    [[nodiscard]] Status try_bind(
        const std::size_t index,
        const std::span<T, Extent> values,
        const bool eof = false) noexcept
    {
        if (values.size_bytes() > std::numeric_limits<std::uint32_t>::max()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return try_bind(
            index,
            std::span<std::uint8_t>{
                reinterpret_cast<std::uint8_t*>(values.data()),
                values.size_bytes(),
            },
            eof);
    }

    void bind(
        const std::size_t index,
        std::span<std::uint8_t> bytes,
        const bool eof = false) noexcept
    {
        static_cast<void>(try_bind(index, bytes, eof));
    }

    [[nodiscard]] constexpr DmaDesc* head() noexcept
    {
        return desc.data();
    }

    [[nodiscard]] constexpr const DmaDesc* head() const noexcept
    {
        return desc.data();
    }

    [[nodiscard]] constexpr DmaDesc* tail() noexcept
    {
        return &desc[N - 1U];
    }

    [[nodiscard]] constexpr const DmaDesc* tail() const noexcept
    {
        return &desc[N - 1U];
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return N;
    }

    [[nodiscard]] constexpr std::size_t bytes() const noexcept
    {
        std::size_t total{};
        for (const auto& item : desc) {
            total += item.length;
        }
        return total;
    }

    std::array<DmaDesc, N> desc{};
};

template <typename T, std::size_t N>
class OwnedDmaChain {
public:
    static_assert(N > 0U, "owned DMA chain must contain at least one descriptor");
    static_assert(std::is_trivially_copyable_v<T>, "owned DMA chain buffers require trivial elements");

    constexpr OwnedDmaChain() noexcept = default;

    OwnedDmaChain(const OwnedDmaChain&) = delete;
    OwnedDmaChain& operator=(const OwnedDmaChain&) = delete;
    OwnedDmaChain(OwnedDmaChain&&) = delete;
    OwnedDmaChain& operator=(OwnedDmaChain&&) = delete;

    [[nodiscard]] Status alloc(
        const std::size_t items,
        const bool circular = false) noexcept
    {
        if (items == 0U || items > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::array<CapsBuf<T>, N> next{};
        for (auto& buffer : next) {
            buffer = dmabuf<T>(items);
            if (!buffer) {
                return fail(ESP_ERR_NO_MEM);
            }
        }

        buffers_ = std::move(next);
        chain_.link_linear();
        for (std::size_t i = 0; i < N; ++i) {
            const auto ready = chain_.try_bind(i, buffers_[i].view(), i + 1U == N && !circular);
            if (!ready) {
                return ready;
            }
        }
        if (circular) {
            chain_.link_circular();
        }
        return ok();
    }

    [[nodiscard]] constexpr DmaChain<N>& chain() noexcept
    {
        return chain_;
    }

    [[nodiscard]] constexpr const DmaChain<N>& chain() const noexcept
    {
        return chain_;
    }

    [[nodiscard]] constexpr DmaDesc* head() noexcept
    {
        return chain_.head();
    }

    [[nodiscard]] constexpr const DmaDesc* head() const noexcept
    {
        return chain_.head();
    }

    [[nodiscard]] constexpr CapsBuf<T>& buf(const std::size_t index) noexcept
    {
        return buffers_[index];
    }

    [[nodiscard]] constexpr const CapsBuf<T>& buf(const std::size_t index) const noexcept
    {
        return buffers_[index];
    }

private:
    DmaChain<N> chain_{};
    std::array<CapsBuf<T>, N> buffers_{};
};

}  // namespace arc
