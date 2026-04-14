#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "arc/task.hpp"

namespace arc {

template <typename Workload>
concept BareWork = requires {
    { Workload::setup() } -> std::same_as<void>;
    { Workload::run() } -> std::same_as<void>;
};

template <typename Workload, typename State>
concept BoundWork = !std::same_as<State, void> && requires(State& state) {
    { Workload::setup(state) } -> std::same_as<void>;
    { Workload::run(state) } -> std::same_as<void>;
};

template <typename Workload,
          std::size_t StackBytes,
          typename State = void,
          Core Bind = Core::core1,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 2)>
struct Plane {
    static_assert(
        BareWork<Workload> || BoundWork<Workload, State>,
        "workload must define setup/run with either no state or a shared state reference");

    static constexpr bool kBound = BoundWork<Workload, State>;
    static constexpr bool kRunNoexcept = []() consteval {
        if constexpr (kBound) {
            return noexcept(Workload::run(std::declval<State&>()));
        } else {
            return noexcept(Workload::run());
        }
    }();

    static_assert(kRunNoexcept, "realtime workload must be noexcept");

    static void boot(const char* tag, const char* name = "rt") requires BareWork<Workload>
    {
        boot(tag, nullptr, name);
    }

    static void boot(const char* tag, State& state, const char* name = "rt") requires kBound
    {
        boot(tag, static_cast<void*>(&state), name);
    }

private:
    static void boot(const char* tag, void* state, const char* name)
    {
        log_boot(tag);

        const auto handle = spawn(
            &entry,
            name,
            state,
            Pri,
            Bind,
            mem);

        configASSERT(handle != nullptr);

        ESP_LOGI(
            tag,
            "rt plane on core %d pri %u",
            static_cast<int>(Bind),
            static_cast<unsigned>(Pri));
    }

    constinit static inline TaskMem<StackBytes> mem{};

    [[nodiscard]] static std::uint32_t psram_free() noexcept
    {
        return static_cast<std::uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    static void log_boot(const char* tag)
    {
        ESP_LOGI(
            tag,
            "boot core=%d internal=%uB psram=%uB",
            esp_cpu_get_core_id(),
            static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
            static_cast<unsigned>(psram_free()));
    }

    static void entry([[maybe_unused]] void* context) noexcept
    {
        if constexpr (kBound) {
            auto& state = *static_cast<State*>(context);
            Workload::setup(state);
            Workload::run(state);
        } else {
            Workload::setup();
            Workload::run();
        }
    }
};

}  // namespace arc
