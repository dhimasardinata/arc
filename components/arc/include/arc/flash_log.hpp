#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"

#include "arc/spsc.hpp"

namespace arc {

template <typename Record, std::size_t Capacity, typename Sink>
struct FlashLog {
    static_assert(std::is_trivially_copyable_v<Record>, "flash log record must be trivially copyable");

    [[nodiscard]] bool push(const Record& record) noexcept
    {
        if (queue.try_push(record)) {
            return true;
        }
        ++dropped_;
        return false;
    }

    [[nodiscard]] esp_err_t flush(const std::size_t max = Capacity - 1U) noexcept
    {
        Record record{};
        std::size_t count{};
        while (count < max && queue.peek(record)) {
            const auto bytes = std::span<const std::uint8_t>{
                reinterpret_cast<const std::uint8_t*>(&record),
                sizeof(record),
            };
            const auto err = sink.write(bytes);
            if (err != ESP_OK) {
                return err;
            }
            static_cast<void>(queue.drop());
            ++count;
        }
        return ESP_OK;
    }

    [[nodiscard]] std::uint32_t dropped() const noexcept
    {
        return dropped_;
    }

    Spsc<Record, Capacity> queue{};
    Sink sink{};

private:
    std::uint32_t dropped_{};
};

}  // namespace arc
