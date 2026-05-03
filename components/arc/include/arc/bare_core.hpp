#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "arc/result.hpp"

namespace arc {

struct BareCoreConfig {
    std::span<std::byte> stack{};
    std::uintptr_t mailbox{};
    bool disable_tick{true};
};

struct BareCoreStubPolicy {
    [[nodiscard]] static esp_err_t boot(
        const BareCoreConfig&,
        void (*)(void*) noexcept,
        void*) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static esp_err_t halt() noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }
};

template <typename Program, typename Policy = BareCoreStubPolicy>
struct BareCore {
    static_assert(std::is_empty_v<Program>, "BareCore program must be static and stateless");

    [[nodiscard]] static Status boot(
        const std::span<std::byte> stack,
        void* const context = nullptr,
        const bool disable_tick = true) noexcept
    {
        if (stack.empty()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const auto cfg = BareCoreConfig{
            .stack = stack,
            .mailbox = reinterpret_cast<std::uintptr_t>(context),
            .disable_tick = disable_tick,
        };
        return status(Policy::boot(cfg, &entry, context));
    }

    [[nodiscard]] static Status halt() noexcept
    {
        return status(Policy::halt());
    }

private:
    static void entry(void* const context) noexcept
    {
        if constexpr (requires { Program::setup(context); }) {
            Program::setup(context);
        }

        for (;;) {
            if constexpr (requires { Program::loop(context); }) {
                Program::loop(context);
            } else {
                Program::loop();
            }
        }
    }
};

}  // namespace arc
