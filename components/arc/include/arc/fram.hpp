#pragma once

#include <bit>
#include <cstddef>
#include <limits>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc {

struct FramSpan {
    std::size_t offset{};
    std::size_t bytes{};

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return bytes == 0U;
    }
};

namespace detail {

[[nodiscard]] constexpr bool fram_can_round(
    const std::size_t value,
    const std::size_t align) noexcept
{
    return align != 0U && value <= (std::numeric_limits<std::size_t>::max() - (align - 1U));
}

[[nodiscard]] constexpr std::size_t fram_round_up(
    const std::size_t value,
    const std::size_t align) noexcept
{
    return (value + align - 1U) & ~(align - 1U);
}

}  // namespace detail

template <typename T>
struct FramRef {
    static_assert(!std::is_const_v<T>,
                  "[ARC ERROR] arc::FramRef value must be mutable. Action: use a non-const trivially copyable type.");
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::FramRef value must be trivially copyable. Action: store flat state, not owning objects.");

    FramSpan span{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return span.bytes == sizeof(T);
    }

    template <typename Policy>
    [[nodiscard]] Status store(const T& value) const noexcept
    {
        if (!valid()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        const auto bytes = std::as_bytes(std::span<const T>{&value, 1U});
        return status(Policy::write(span.offset, bytes));
    }

    template <typename Policy>
    [[nodiscard]] Result<T> load() const noexcept
    {
        if (!valid()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        T value;
        auto bytes = std::as_writable_bytes(std::span<T>{&value, 1U});
        const auto err = Policy::read(span.offset, bytes);
        if (err != ESP_OK) {
            return fail(err);
        }
        return value;
    }
};

template <std::size_t Bytes, std::size_t Align = alignof(std::max_align_t)>
struct FramArena {
    static_assert(Bytes > 0U,
                  "[ARC ERROR] arc::FramArena needs storage. Action: set Bytes to the external FRAM/MRAM byte budget.");
    static_assert(std::has_single_bit(Align),
                  "[ARC ERROR] arc::FramArena alignment must be a power of two. Action: use 1, 2, 4, 8, ...");

    std::size_t used{};

    [[nodiscard]] static constexpr std::size_t capacity() noexcept
    {
        return Bytes;
    }

    [[nodiscard]] constexpr std::size_t free() const noexcept
    {
        return used <= Bytes ? Bytes - used : 0U;
    }

    constexpr void reset() noexcept
    {
        used = 0U;
    }

    [[nodiscard]] constexpr Result<FramSpan> alloc(
        const std::size_t bytes,
        const std::size_t align = Align) noexcept
    {
        if (bytes == 0U || !std::has_single_bit(align)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (used > Bytes || !detail::fram_can_round(used, align)) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto offset = detail::fram_round_up(used, align);
        if (offset > Bytes || bytes > Bytes - offset) {
            return fail(ESP_ERR_NO_MEM);
        }

        used = offset + bytes;
        return FramSpan{.offset = offset, .bytes = bytes};
    }

    template <typename T>
    [[nodiscard]] constexpr Result<FramRef<T>> make() noexcept
    {
        constexpr auto type_align = alignof(T) > Align ? alignof(T) : Align;
        ARC_TRY(span, alloc(sizeof(T), type_align));
        return FramRef<T>{.span = span};
    }
};

template <std::size_t Bytes, std::size_t Align = alignof(std::max_align_t)>
using FramAlloc = FramArena<Bytes, Align>;

}  // namespace arc
