#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "esp_err.h"

#include "arc/log.hpp"
#include "arc/result.hpp"

namespace arc {

struct TraceEventWriter {
    [[nodiscard]] static Result<std::span<const char>> json_begin(std::span<char> out) noexcept
    {
        std::size_t pos{};
        if (!append(out, pos, "{\"traceEvents\":[")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
    }

    [[nodiscard]] static Result<std::span<const char>> json_end(std::span<char> out) noexcept
    {
        std::size_t pos{};
        if (!append(out, pos, "]}")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
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

        std::size_t pos{};
        if ((comma && !append(out, pos, ",")) || !append(out, pos, "{\"name\":\"") ||
            !append_escaped(out, pos, name) || !append(out, pos, "\",\"cat\":\"arc\",\"ph\":\"i\",\"s\":\"t\",\"ts\":") ||
            !append_u64(out, pos, event.tick) || !append(out, pos, ",\"pid\":0,\"tid\":") ||
            !append_u32(out, pos, event.aux) || !append(out, pos, ",\"args\":{\"id\":") ||
            !append_u32(out, pos, event.id) || !append(out, pos, ",\"payload\":") ||
            !append_u32(out, pos, event.payload) || !append(out, pos, "}}")) {
            return fail(ESP_ERR_NO_MEM);
        }
        return out.first(pos);
    }

private:
    [[nodiscard]] static bool append(std::span<char> out, std::size_t& pos, const char* const text) noexcept
    {
        const auto bytes = std::strlen(text);
        if (out.size() - pos < bytes) {
            return false;
        }
        std::memcpy(out.data() + pos, text, bytes);
        pos += bytes;
        return true;
    }

    [[nodiscard]] static bool append_escaped(std::span<char> out, std::size_t& pos, const char* text) noexcept
    {
        while (*text != '\0') {
            const char ch = *text++;
            if (ch == '"' || ch == '\\') {
                if (out.size() - pos < 2U) {
                    return false;
                }
                out[pos++] = '\\';
                out[pos++] = ch;
            } else {
                if (out.size() - pos < 1U) {
                    return false;
                }
                out[pos++] = ch;
            }
        }
        return true;
    }

    [[nodiscard]] static bool append_u32(std::span<char> out, std::size_t& pos, const std::uint32_t value) noexcept
    {
        return append_u64(out, pos, value);
    }

    [[nodiscard]] static bool append_u64(std::span<char> out, std::size_t& pos, std::uint64_t value) noexcept
    {
        char tmp[20]{};
        std::size_t count{};
        do {
            tmp[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);

        if (out.size() - pos < count) {
            return false;
        }
        while (count != 0U) {
            out[pos++] = tmp[--count];
        }
        return true;
    }
};

}  // namespace arc
