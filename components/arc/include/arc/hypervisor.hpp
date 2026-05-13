#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/mmu_span.hpp"
#include "arc/result.hpp"
#include "arc/tee.hpp"

namespace arc::vm {

enum class TrapKind : std::uint8_t {
    memory,
    peripheral,
    interrupt,
    illegal_instruction,
    budget,
};

enum class TrapAction : std::uint8_t {
    resume,
    emulate,
    terminate,
};

struct TrapFrame {
    TrapKind kind{};
    std::uintptr_t pc{};
    std::uintptr_t address{};
    std::uint32_t source{};
    std::uint32_t value{};
    std::uint32_t core{};
};

struct TrapDecision {
    TrapAction action{TrapAction::terminate};
    std::uint32_t value{};
};

struct Partition {
    std::span<const std::uint8_t> code{};
    std::span<std::uint8_t> ram{};
    std::span<const std::uint32_t> trusted_peripherals{};
    std::span<const std::uint32_t> untrusted_peripherals{};
    std::uint32_t trusted_core{1U};
    std::uint32_t untrusted_core{0U};
};

struct Hypervisor {
    [[nodiscard]] static bool valid(const Partition& partition) noexcept
    {
        return !partition.code.empty() && !partition.ram.empty() &&
            partition.trusted_core != partition.untrusted_core;
    }

    template <typename Policy>
    [[nodiscard]] static Status apply(const Partition& partition) noexcept
    {
        if (!valid(partition)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const WorldRegion regions[2]{
            {
                .base = partition.code.data(),
                .bytes = partition.code.size_bytes(),
                .owner = World::untrusted,
                .trusted = PmsAccess::rwx,
                .untrusted = PmsAccess::read_execute,
            },
            {
                .base = partition.ram.data(),
                .bytes = partition.ram.size_bytes(),
                .owner = World::untrusted,
                .trusted = PmsAccess::read_write,
                .untrusted = PmsAccess::read_write,
            },
        };
        const TeePlan plan{
            .regions = std::span<const WorldRegion>{regions},
            .trusted_peripherals = partition.trusted_peripherals,
            .untrusted_peripherals = partition.untrusted_peripherals,
            .trusted_core = partition.trusted_core,
            .untrusted_core = partition.untrusted_core,
        };
        auto applied = WorldGuard<Policy>::apply(plan);
        if (!applied) {
            return applied;
        }

        if constexpr (requires { Policy::bind_traps(); }) {
            return status(Policy::bind_traps());
        } else {
            return ok();
        }
    }

    template <typename Policy>
    [[nodiscard]] static Status launch(
        const Partition& partition,
        void (*entry)(void*) noexcept,
        void* const arg = nullptr) noexcept
    {
        if (entry == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        auto applied = apply<Policy>(partition);
        if (!applied) {
            return applied;
        }
        if constexpr (requires { Policy::start_core0(entry, arg); }) {
            return status(Policy::start_core0(entry, arg));
        } else {
            return ok();
        }
    }

    template <typename Policy>
    [[nodiscard]] static Result<TrapDecision> trap(const TrapFrame& frame) noexcept
    {
        if constexpr (requires { Policy::handle_trap(frame); }) {
            return Policy::handle_trap(frame);
        } else if constexpr (requires { Policy::emulate(frame); }) {
            return TrapDecision{
                .action = TrapAction::emulate,
                .value = Policy::emulate(frame),
            };
        } else {
            if constexpr (requires { Policy::terminate(frame); }) {
                if (const auto err = Policy::terminate(frame); err != ESP_OK) {
                    return fail(err);
                }
            }
            return TrapDecision{.action = TrapAction::terminate};
        }
    }
};

}  // namespace arc::vm
