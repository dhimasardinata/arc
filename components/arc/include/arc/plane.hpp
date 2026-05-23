#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "arc/borrow.hpp"
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
    static constexpr std::size_t stack_bytes = StackBytes;
    static constexpr std::size_t stack_required = stack::required<Workload, State>();
    static_assert(
        stack::fits<stack_bytes, stack_required>(),
        "Plane StackBytes is smaller than Workload::stack_bytes compile-time budget");

    static constexpr bool kBound = BoundWork<Workload, State>;
    static constexpr bool kRunNoexcept = []() consteval {
        if constexpr (kBound) {
            return noexcept(Workload::run(std::declval<State&>()));
        } else {
            return noexcept(Workload::run());
        }
    }();

    static_assert(kRunNoexcept, "realtime workload must be noexcept");

    static void boot(const char* tag, const char* name = "rt")
        requires(BareWork<Workload> && !kBound)
    {
        boot(tag, nullptr, name);
    }

    template <auto* Shared>
    static void boot(const char* tag, const char* name = "rt")
        requires(kBound && StaticTaskState<Shared, State>)
    {
        boot(tag, static_cast<void*>(Shared), name);
    }

private:
    struct Launch {
        TaskHandle_t task{};
        std::uint32_t active{};
        std::uint32_t parked{};
    };

    static void boot(const char* tag, void* state, const char* name)
    {
        static_cast<void>(reap());

        auto inactive = std::uint32_t{};
        if (!__atomic_compare_exchange_n(
                &launch.active,
                &inactive,
                1U,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return;
        }

        __atomic_store_n(&launch.parked, 0U, __ATOMIC_RELEASE);
        log_boot(tag);

        const auto handle = spawn(
            &entry,
            name,
            state,
            Pri,
            Bind,
            mem);

        set_task(handle);
        if (handle == nullptr) {
            set_task(nullptr);
            __atomic_store_n(&launch.active, 0U, __ATOMIC_RELEASE);
            __atomic_store_n(&launch.parked, 0U, __ATOMIC_RELEASE);
        }
        configASSERT(handle != nullptr);

        ESP_LOGI(
            tag,
            "rt plane on core %d pri %u",
            static_cast<int>(Bind),
            static_cast<unsigned>(Pri));
    }

    constinit static inline TaskMem<StackBytes, stack_required> mem{};
    constinit static inline Launch launch{};

    static void set_task(const TaskHandle_t task) noexcept
    {
        __atomic_store_n(&launch.task, task, __ATOMIC_RELEASE);
    }

    [[nodiscard]] static std::uint32_t psram_free() noexcept
    {
        return static_cast<std::uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }

    [[nodiscard]] static bool reap() noexcept
    {
        if (__atomic_load_n(&launch.active, __ATOMIC_ACQUIRE) == 0U) {
            return true;
        }

        auto parked = std::uint32_t{1U};
        if (!__atomic_compare_exchange_n(
                &launch.parked,
                &parked,
                2U,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE)) {
            return false;
        }

        const auto handle = __atomic_load_n(&launch.task, __ATOMIC_ACQUIRE);
        configASSERT(handle == nullptr || xTaskGetCurrentTaskHandle() != handle);
        if (handle != nullptr) {
            vTaskDelete(handle);
        }

        set_task(nullptr);
        __atomic_store_n(&launch.active, 0U, __ATOMIC_RELEASE);
        __atomic_store_n(&launch.parked, 0U, __ATOMIC_RELEASE);
        return true;
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
        set_task(xTaskGetCurrentTaskHandle());

        if constexpr (kBound) {
            auto& state = *static_cast<State*>(context);
            Workload::setup(state);
            Workload::run(state);
        } else {
            Workload::setup();
            Workload::run();
        }

        __atomic_store_n(&launch.parked, 1U, __ATOMIC_RELEASE);
        park_task();
    }
};

template <typename Workload,
          StaticRefType StateRef,
          std::size_t StackBytes,
          Core Bind = StateRef::owner == Core::any ? CoreMap<>::det : StateRef::owner,
          UBaseType_t Pri = static_cast<UBaseType_t>(configMAX_PRIORITIES - 2)>
struct StaticPlane {
    using State = typename StateRef::value_type;
    using Bound = Plane<Workload, StackBytes, State, Bind, Pri>;

    static_assert(StateRef::owner == Core::any || StateRef::owner == Bind,
                  "[ARC ERROR] arc::StaticPlane owner mismatch. Action: keep StaticRef owner and plane core aligned.");
    static_assert(!std::is_const_v<std::remove_pointer_t<decltype(StateRef::object)>>,
                  "[ARC ERROR] arc::StaticPlane needs mutable static state. Action: use StaticRef over non-const storage.");

    static constexpr Core core = Bind;
    static constexpr std::size_t stack_bytes = Bound::stack_bytes;
    static constexpr std::size_t stack_required = Bound::stack_required;

    static void boot(const char* tag, const char* name = "rt")
    {
        Bound::template boot<StateRef::object>(tag, name);
    }
};

}  // namespace arc
