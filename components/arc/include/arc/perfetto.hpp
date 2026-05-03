#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

#include "arc/log.hpp"
#include "arc/result.hpp"

namespace arc {

struct PerfettoWriter {
    [[nodiscard]] static Result<std::span<const std::uint8_t>> event(
        std::span<std::uint8_t> out,
        const LogEvent event) noexcept
    {
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

}  // namespace arc
