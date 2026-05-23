#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

#include "arc/log.hpp"
#include "arc/result.hpp"

namespace arc {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool perfetto_valid_span(const std::span<T, Extent> value) noexcept
{
    return value.empty() || value.data() != nullptr;
}

}  // namespace detail

struct PerfettoWriter {
    [[nodiscard]] static Result<std::span<const std::uint8_t>> event(
        std::span<std::uint8_t> out,
        const LogEvent event) noexcept
    {
        if (!detail::perfetto_valid_span(out)) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t pos{};
        if (!varint(out, pos, 0x08U) || !varint(out, pos, event.tick) ||
            !varint(out, pos, 0x10U) || !varint(out, pos, event.id) ||
            !varint(out, pos, 0x18U) || !varint(out, pos, event.payload) ||
            !varint(out, pos, 0x20U) || !varint(out, pos, event.aux)) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
    }

private:
    [[nodiscard]] static bool varint(
        std::span<std::uint8_t> out,
        std::size_t& pos,
        std::uint64_t value) noexcept
    {
        do {
            if (pos >= out.size()) {
                return false;
            }
            auto byte = static_cast<std::uint8_t>(value & 0x7fU);
            value >>= 7U;
            if (value != 0U) {
                byte |= 0x80U;
            }
            out[pos++] = byte;
        } while (value != 0U);
        return true;
    }
};

template <std::size_t ScratchBytes, typename Sink>
struct PerfettoStream {
    static_assert(ScratchBytes >= 32U, "perfetto stream scratch buffer is too small");

    template <std::size_t LaneCapacity>
    [[nodiscard]] esp_err_t drain(LogLane<LaneCapacity>& lane) noexcept
    {
        const auto result = drain_some(lane, LogLane<LaneCapacity>::cap());
        return result ? ESP_OK : result.error();
    }

    template <std::size_t LaneCapacity>
    [[nodiscard]] Result<std::size_t> drain_some(
        LogLane<LaneCapacity>& lane,
        const std::size_t max_events) noexcept
    {
        if (failed != ESP_OK) {
            return fail(failed);
        }
        if (max_events == 0U) {
            return 0U;
        }

        LogEvent item{};
        auto count = std::size_t{};
        while (count < max_events && lane.peek(item)) {
            const auto encoded = PerfettoWriter::event(std::span(scratch), item);
            if (!encoded) {
                failed = encoded.error();
                return fail(failed);
            }
            const auto err = sink.write(*encoded);
            if (err != ESP_OK) {
                failed = err;
                return fail(failed);
            }
            static_cast<void>(lane.drop());
            ++count;
        }
        return count;
    }

    Sink sink{};

private:
    std::array<std::uint8_t, ScratchBytes> scratch{};
    esp_err_t failed{ESP_OK};
};

}  // namespace arc
