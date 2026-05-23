#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "esp_err.h"

#include "arc/log.hpp"
#include "arc/result.hpp"
#include "arc/trace_event.hpp"

namespace arc {

template <std::size_t ScratchBytes, typename Sink>
struct TraceStream {
    static_assert(ScratchBytes >= 96U, "trace stream scratch buffer is too small");

    template <std::size_t LaneCapacity>
    [[nodiscard]] esp_err_t drain(
        LogLane<LaneCapacity>& lane,
        const char* const name = "arc") noexcept
    {
        const auto result = drain_some(lane, LogLane<LaneCapacity>::cap(), name);
        return result ? ESP_OK : result.error();
    }

    template <std::size_t LaneCapacity>
    [[nodiscard]] Result<std::size_t> drain_some(
        LogLane<LaneCapacity>& lane,
        const std::size_t max_events,
        const char* const name = "arc") noexcept
    {
        if (name == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (failed != ESP_OK) {
            return fail(failed);
        }
        if (max_events == 0U) {
            return 0U;
        }

        auto comma = started;
        if (!started) {
            const auto begin = TraceEventWriter::json_begin(std::span(scratch));
            if (!begin) {
                return fail(begin.error());
            }
            const auto err = sink.write(*begin);
            if (err != ESP_OK) {
                return fail(err);
            }
            started = true;
        }

        LogEvent event{};
        auto count = std::size_t{};
        while (count < max_events && lane.peek(event)) {
            const auto encoded = TraceEventWriter::json_event(std::span(scratch), event, comma, name);
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
            comma = true;
            ++count;
        }
        return count;
    }

    [[nodiscard]] esp_err_t finish() noexcept
    {
        if (failed != ESP_OK) {
            return failed;
        }

        if (!started) {
            const auto begin = TraceEventWriter::json_begin(std::span(scratch));
            if (!begin) {
                return begin.error();
            }
            const auto err = sink.write(*begin);
            if (err != ESP_OK) {
                return err;
            }
            started = true;
        }

        const auto end = TraceEventWriter::json_end(std::span(scratch));
        if (!end) {
            return end.error();
        }
        const auto err = sink.write(*end);
        if (err == ESP_OK) {
            started = false;
            failed = ESP_OK;
        }
        return err;
    }

    Sink sink{};
    bool started{};

private:
    std::array<char, ScratchBytes> scratch{};
    esp_err_t failed{ESP_OK};
};

}  // namespace arc
