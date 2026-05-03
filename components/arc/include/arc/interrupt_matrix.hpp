#pragma once

#include <cstdint>

#include "arc/result.hpp"

namespace arc {

struct InterruptBinding {
    int source{};
    std::uint8_t cpu_interrupt{};
    std::uint8_t level{};
};

struct InterruptMatrixStubPolicy {
    [[nodiscard]] static esp_err_t bind(const InterruptBinding&, void (*)(void*) noexcept, void*) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static esp_err_t unbind(const InterruptBinding&) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }
};

template <int Source, std::uint8_t CpuInterrupt, std::uint8_t Level, auto Handler, typename Policy = InterruptMatrixStubPolicy>
struct InterruptMatrix {
    static constexpr auto binding = InterruptBinding{
        .source = Source,
        .cpu_interrupt = CpuInterrupt,
        .level = Level,
    };

    [[nodiscard]] static Status bind(void* const arg = nullptr) noexcept
    {
        return status(Policy::bind(binding, &isr, arg));
    }

    [[nodiscard]] static Status unbind() noexcept
    {
        return status(Policy::unbind(binding));
    }

    static void isr(void* const arg) noexcept
    {
        if constexpr (requires { Handler(arg); }) {
            Handler(arg);
        } else {
            Handler();
        }
    }
};

template <std::uint8_t Level, auto Handler>
struct RawVector {
    static_assert(Level > 0U && Level <= 7U, "Xtensa interrupt level must be 1..7");

    static void invoke(void* const arg = nullptr) noexcept
    {
        if constexpr (requires { Handler(arg); }) {
            Handler(arg);
        } else {
            Handler();
        }
    }
};

}  // namespace arc
