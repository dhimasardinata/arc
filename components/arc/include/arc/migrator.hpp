#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "arc/power_governor.hpp"
#include "arc/probe.hpp"
#include "arc/result.hpp"
#include "arc/wasm_aot.hpp"

namespace arc::swarm {

struct WasmSnapshot {
    std::span<const std::uint8_t> linear_memory{};
    std::uint32_t stack_pointer{};
    std::uint32_t instruction_pointer{};
    std::uint32_t fuel{};
};

struct MigrationDecision {
    bool migrate{};
    std::uint16_t peer{};
    std::int32_t slack{};
    std::uint32_t overruns{};
};

template <std::size_t MaxBytes>
struct MigrationFrame {
    static_assert(MaxBytes > 0U, "migration frame needs payload storage");

    std::uint32_t process_id{};
    std::uint32_t instruction_pointer{};
    std::uint32_t stack_pointer{};
    std::uint32_t fuel{};
    std::uint32_t memory_bytes{};
    std::array<std::uint8_t, MaxBytes> memory{};
};

struct Migrator {
    [[nodiscard]] static constexpr MigrationDecision from_governor(
        const power::GovernorDecision decision,
        const std::uint16_t peer) noexcept
    {
        return {
            .migrate = peer != 0U &&
                (decision.action == power::GovernorAction::boost || decision.overrun_risk),
            .peer = peer,
            .slack = decision.predicted_slack,
            .overruns = decision.overrun_risk ? 1U : 0U,
        };
    }

    [[nodiscard]] static constexpr MigrationDecision from_deadline(
        const DeadlineStats& stats,
        const std::int32_t min_slack,
        const std::uint16_t peer) noexcept
    {
        return {
            .migrate = peer != 0U && stats.samples != 0U &&
                (stats.min_slack <= min_slack || stats.overruns != 0U),
            .peer = peer,
            .slack = stats.avg_slack(),
            .overruns = stats.overruns,
        };
    }

    template <std::size_t MaxBytes>
    [[nodiscard]] static Result<MigrationFrame<MaxBytes>> snapshot(
        const std::uint32_t process_id,
        const WasmSnapshot state) noexcept
    {
        if (process_id == 0U || state.linear_memory.empty() || state.linear_memory.size() > MaxBytes) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        MigrationFrame<MaxBytes> frame{
            .process_id = process_id,
            .instruction_pointer = state.instruction_pointer,
            .stack_pointer = state.stack_pointer,
            .fuel = state.fuel,
            .memory_bytes = static_cast<std::uint32_t>(state.linear_memory.size()),
        };
        std::memcpy(frame.memory.data(), state.linear_memory.data(), state.linear_memory.size());
        return frame;
    }

    template <typename Policy, std::size_t MaxBytes>
    [[nodiscard]] static Status teleport(
        const std::uint16_t peer,
        const MigrationFrame<MaxBytes>& frame) noexcept
    {
        if (peer == 0U || frame.process_id == 0U || frame.memory_bytes == 0U || frame.memory_bytes > MaxBytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        return status(Policy::send(peer, bytes(frame)));
    }

    template <typename Policy, std::size_t MaxBytes>
    [[nodiscard]] static Status resume(
        const MigrationFrame<MaxBytes>& frame,
        const std::span<std::uint8_t> linear_memory) noexcept
    {
        if (frame.process_id == 0U || frame.memory_bytes == 0U || frame.memory_bytes > MaxBytes ||
            linear_memory.size() < frame.memory_bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        std::memcpy(linear_memory.data(), frame.memory.data(), frame.memory_bytes);
        return status(Policy::resume_wasm(
            frame.process_id,
            frame.instruction_pointer,
            frame.stack_pointer,
            linear_memory.first(frame.memory_bytes)));
    }

    template <std::size_t MaxBytes>
    [[nodiscard]] static std::span<const std::uint8_t> bytes(
        const MigrationFrame<MaxBytes>& frame) noexcept
    {
        const auto header = offsetof(MigrationFrame<MaxBytes>, memory);
        return {
            reinterpret_cast<const std::uint8_t*>(&frame),
            header + frame.memory_bytes,
        };
    }
};

}  // namespace arc::swarm
