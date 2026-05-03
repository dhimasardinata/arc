#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"

#include "arc/ethernet.hpp"

namespace arc::net {

struct W5500MacRaw {
    static constexpr std::uint16_t tx_base = 0x4000U;
    static constexpr std::uint16_t rx_base = 0x6000U;
    static constexpr std::uint8_t write = 0x04U;
    static constexpr std::uint8_t read = 0x00U;

    [[nodiscard]] static constexpr std::array<std::uint8_t, 3> header(
        const std::uint16_t address,
        const bool wr) noexcept
    {
        return {
            static_cast<std::uint8_t>(address >> 8U),
            static_cast<std::uint8_t>(address),
            static_cast<std::uint8_t>(wr ? write : read),
        };
    }
};

template <typename SpiPolicy, std::size_t Mtu = 1536U, std::size_t Capacity = 4U>
struct W5500Raw {
    EthernetRing<Mtu, Capacity> ring{};

    [[nodiscard]] esp_err_t send(std::span<const std::uint8_t> frame) noexcept
    {
        if (frame.size() > Mtu) {
            return ESP_ERR_INVALID_ARG;
        }
        return SpiPolicy::writev(W5500MacRaw::header(W5500MacRaw::tx_base, true), frame);
    }

    [[nodiscard]] esp_err_t ingest(std::span<const std::uint8_t> frame) noexcept
    {
        return ring.push(frame) ? ESP_OK : ESP_ERR_NO_MEM;
    }
};

}  // namespace arc::net
