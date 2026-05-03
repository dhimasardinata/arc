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
inline constexpr std::uint32_t postmortem_version = 2U;
inline constexpr std::size_t postmortem_trace_depth = 16U;

struct PostmortemHeader {
    std::uint32_t magic{};
    std::uint32_t version{};
    std::uint32_t boots{};
    std::uint32_t writes{};
    std::uint32_t dropped{};
    std::uint32_t capacity{};
};

static_assert(std::is_trivially_copyable_v<PostmortemHeader>);

struct PostmortemFaultFrame {
    std::uint32_t cause{};
    std::uintptr_t pc{};
    std::uintptr_t ps{};
    std::uintptr_t sp{};
    std::uintptr_t a0{};
    std::uintptr_t excvaddr{};
    std::uint32_t core{};
    std::uint32_t frames{};
    std::array<std::uintptr_t, postmortem_trace_depth> trace{};
};

static_assert(std::is_trivially_copyable_v<PostmortemFaultFrame>);

template <std::size_t Capacity>
struct Postmortem {
    static_assert(Capacity > 0U, "postmortem capacity must be non-zero");
    static_assert(std::has_single_bit(Capacity), "postmortem capacity must be a power of two");

    struct Store {
        PostmortemHeader header{};
        std::uint32_t fault_valid{};
        PostmortemFaultFrame fault{};
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

    [[nodiscard]] static bool has_fault() noexcept
    {
        return valid() && store.fault_valid != 0U;
    }

    [[nodiscard]] static const PostmortemFaultFrame& fault() noexcept
    {
        return store.fault;
    }

    static void clear_fault() noexcept
    {
        store.fault_valid = 0U;
        store.fault = {};
    }

    static void capture_fault(PostmortemFaultFrame frame) noexcept
    {
        if (!valid()) {
            clear();
        }
        if (frame.frames > postmortem_trace_depth) {
            frame.frames = postmortem_trace_depth;
        }
        store.fault = frame;
        store.fault_valid = 1U;
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
