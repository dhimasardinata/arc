#pragma once

#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace arc::detail {

struct EndpointOwner {
    [[nodiscard]] bool claim() noexcept
    {
        const auto current = token();
        auto expected = std::uintptr_t{};
        if (__atomic_compare_exchange_n(
                &owner_,
                &expected,
                current,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return true;
        }

        return expected == current;
    }

    void assert_single(const char* const message) noexcept
    {
        configASSERT(claim() && message);
    }

private:
    [[nodiscard]] static std::uintptr_t token() noexcept
    {
#if defined(ESP_PLATFORM)
        if (xPortInIsrContext()) {
            return 1U + static_cast<std::uintptr_t>(xPortGetCoreID());
        }

        const auto handle = xTaskGetCurrentTaskHandle();
        return handle != nullptr ? reinterpret_cast<std::uintptr_t>(handle) : 1U;
#else
        static thread_local int slot;
        return reinterpret_cast<std::uintptr_t>(&slot);
#endif
    }

    std::uintptr_t owner_{0U};
};

}  // namespace arc::detail
