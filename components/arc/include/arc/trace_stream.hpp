#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "esp_err.h"

#include "arc/log.hpp"
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
        auto comma = started;
        auto count = std::size_t{};
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

        const auto drained = lane.drain([&](const LogEvent& event) noexcept {
            if (failed != ESP_OK) {
                return;
            }
            const auto encoded = TraceEventWriter::json_event(std::span(scratch), event, comma, name);
            if (!encoded) {
                failed = encoded.error();
                return;
            }
            failed = sink.write(*encoded);
            comma = true;
            ++count;
        });
        static_cast<void>(drained);
        return failed == ESP_OK ? ESP_OK : failed;
    }

    [[nodiscard]] esp_err_t finish() noexcept
    {
        const auto end = TraceEventWriter::json_end(std::span(scratch));
        if (!end) {
            return end.error();
        }
        started = false;
        failed = ESP_OK;
        return sink.write(*end);
    }

    Sink sink{};
    bool started{};

private:
    std::array<char, ScratchBytes> scratch{};
    esp_err_t failed{ESP_OK};
};

}  // namespace arc
