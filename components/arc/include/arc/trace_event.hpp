#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "esp_err.h"

#include "arc/log.hpp"
#include "arc/result.hpp"
#include "arc/text.hpp"

namespace arc {

struct TraceEventWriter {
    [[nodiscard]] static Result<std::span<const char>> json_begin(std::span<char> out) noexcept
    {
        Text text{out};
        if (!text.append("{\"traceEvents\":[")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return text.span();
    }

    [[nodiscard]] static Result<std::span<const char>> json_end(std::span<char> out) noexcept
    {
        Text text{out};
        if (!text.append("]}")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return text.span();
    }

    [[nodiscard]] static Result<std::span<const char>> json_event(
        std::span<char> out,
        const LogEvent& event,
        const bool comma = false,
        const char* const name = "arc") noexcept
    {
        if (name == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        Text text{out};
        if ((comma && !text.push(',')) || !text.append("{\"name\":\"") ||
            !text.json(std::string_view{name}) || !text.append("\",\"cat\":\"arc\",\"ph\":\"i\",\"s\":\"t\",\"ts\":") ||
            !text.u64(event.tick) || !text.append(",\"pid\":0,\"tid\":") ||
            !text.u32(event.aux) || !text.append(",\"args\":{\"id\":") ||
            !text.u32(event.id) || !text.append(",\"payload\":") ||
            !text.u32(event.payload) || !text.append("}}")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return text.span();
    }
};

}  // namespace arc
