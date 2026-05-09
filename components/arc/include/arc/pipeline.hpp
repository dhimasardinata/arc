#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/dma_chain.hpp"
#include "arc/result.hpp"

namespace arc {

struct DmaEndpoint {
    DmaDesc* head{};
    DmaDesc* tail{};
    std::size_t descriptors{};
    std::size_t bytes{};

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return head != nullptr && tail != nullptr && descriptors != 0U;
    }
};

template <std::size_t N>
[[nodiscard]] constexpr DmaEndpoint endpoint(DmaChain<N>& chain) noexcept
{
    return {
        .head = chain.head(),
        .tail = chain.tail(),
        .descriptors = N,
        .bytes = chain.bytes(),
    };
}

template <std::size_t Stages>
struct Pipeline {
    static_assert(Stages > 1U, "pipeline needs at least source and sink stages");

    std::array<DmaEndpoint, Stages> stages{};

    [[nodiscard]] Status bind(const std::size_t index, const DmaEndpoint stage) noexcept
    {
        if (index >= Stages || !stage) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        stages[index] = stage;
        return ok();
    }

    [[nodiscard]] Status link_linear() noexcept
    {
        for (const auto& stage : stages) {
            if (!stage) {
                return Status{fail(ESP_ERR_INVALID_STATE)};
            }
        }

        for (std::size_t i = 0; i + 1U < Stages; ++i) {
            stages[i].tail->next = stages[i + 1U].head;
        }
        stages[Stages - 1U].tail->next = nullptr;
        return ok();
    }

    [[nodiscard]] Status link_ring() noexcept
    {
        auto linked = link_linear();
        if (!linked) {
            return linked;
        }
        stages[Stages - 1U].tail->next = stages[0].head;
        return ok();
    }

    [[nodiscard]] constexpr DmaEndpoint source() const noexcept
    {
        return stages[0];
    }

    [[nodiscard]] constexpr DmaEndpoint sink() const noexcept
    {
        return stages[Stages - 1U];
    }
};

struct Dma2dWindow {
    std::size_t x_bytes{};
    std::size_t y{};
    std::size_t width_bytes{};
    std::size_t height{};
    std::size_t stride_bytes{};
};

[[nodiscard]] constexpr bool valid(const Dma2dWindow window) noexcept
{
    return window.width_bytes != 0U &&
        window.height != 0U &&
        window.stride_bytes != 0U &&
        window.x_bytes + window.width_bytes <= window.stride_bytes;
}

template <std::size_t Rows>
[[nodiscard]] Status bind_rows(
    DmaChain<Rows>& chain,
    const std::span<std::uint8_t> frame,
    const Dma2dWindow window,
    const bool circular = false) noexcept
{
    if (!valid(window) || window.height > Rows) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    const auto last = window.y + window.height - 1U;
    if ((last * window.stride_bytes) + window.x_bytes + window.width_bytes > frame.size()) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    for (std::size_t row = 0; row < window.height; ++row) {
        const auto offset = ((window.y + row) * window.stride_bytes) + window.x_bytes;
        chain.bind(row, frame.subspan(offset, window.width_bytes), row + 1U == window.height && !circular);
    }

    for (std::size_t row = window.height; row < Rows; ++row) {
        chain.desc[row] = {};
    }

    if (circular) {
        chain.desc[window.height - 1U].next = chain.head();
    } else {
        chain.desc[window.height - 1U].next = nullptr;
    }
    return ok();
}

}  // namespace arc
