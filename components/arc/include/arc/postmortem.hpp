#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "arc/log.hpp"
#include "arc/place.hpp"

namespace arc {

inline constexpr std::uint32_t postmortem_magic = 0x4152'4350U;
inline constexpr std::uint32_t postmortem_version = 1U;

struct PostmortemHeader {
    std::uint32_t magic{};
    std::uint32_t version{};
    std::uint32_t boots{};
    std::uint32_t writes{};
    std::uint32_t dropped{};
    std::uint32_t capacity{};
};

static_assert(std::is_trivially_copyable_v<PostmortemHeader>);

template <std::size_t Capacity>
struct Postmortem {
    static_assert(Capacity > 0U, "postmortem capacity must be non-zero");
    static_assert(std::has_single_bit(Capacity), "postmortem capacity must be a power of two");

    struct Store {
        PostmortemHeader header{};
        std::array<LogEvent, Capacity> events{};
    };

    static_assert(std::is_trivially_copyable_v<Store>);

    static void boot() noexcept
    {
        if (!valid()) {
            clear();
        }
        store.header.boots += 1U;
    }

    static void clear() noexcept
    {
        store = Store{};
        store.header.magic = postmortem_magic;
        store.header.version = postmortem_version;
        store.header.capacity = Capacity;
    }

    [[nodiscard]] static bool valid() noexcept
    {
        return store.header.magic == postmortem_magic &&
            store.header.version == postmortem_version &&
            store.header.capacity == Capacity;
    }

    [[nodiscard]] static const PostmortemHeader& header() noexcept
    {
        return store.header;
    }

    [[nodiscard]] static std::size_t size() noexcept
    {
        const auto writes = store.header.writes;
        return writes < Capacity ? writes : Capacity;
    }

    [[nodiscard]] static const LogEvent& event(const std::size_t index) noexcept
    {
        const auto used = size();
        if (used == 0U) {
            return store.events[0];
        }
        const auto first = store.header.writes > Capacity ? store.header.writes - Capacity : 0U;
        const auto pos = (first + (index < used ? index : used - 1U)) & (Capacity - 1U);
        return store.events[pos];
    }

    static void append(const LogEvent event) noexcept
    {
        if (!valid()) {
            clear();
        }
        const auto pos = store.header.writes & (Capacity - 1U);
        store.events[pos] = event;
        store.header.writes += 1U;
    }

    template <std::size_t LaneCapacity>
    static std::size_t capture(LogLane<LaneCapacity>& lane) noexcept
    {
        auto count = std::size_t{};
        count += lane.drain([](const LogEvent& event) noexcept {
            append(event);
        });
        store.header.dropped = lane.dropped();
        return count;
    }

private:
    ARC_RTC_NOINIT constinit static inline Store store{};
};

}  // namespace arc
