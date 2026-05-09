#pragma once

#include <cstdint>

#include "arc/result.hpp"

namespace arc {

enum class IpcCore : std::uint8_t {
    core0,
    core1,
};

struct Ipc {
    using Fn = void (*)(void*) noexcept;

    template <typename Policy>
    [[nodiscard]] static Status call(const IpcCore core, Fn fn, void* arg = nullptr) noexcept
    {
        if (fn == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::ipc_call(core, fn, arg));
    }

    template <typename Policy>
    [[nodiscard]] static Status emergency_halt(const IpcCore core) noexcept
    {
        return status(Policy::ipc_halt(core));
    }
};

}  // namespace arc
