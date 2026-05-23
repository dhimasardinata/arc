#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "arc/result.hpp"
#include "arc/timesync.hpp"

namespace arc::net {

struct TsnGate {
    std::uint64_t offset_ns{};
    std::uint64_t duration_ns{};
    std::uint32_t guard_ns{};
    std::uint8_t traffic{};
};

struct TsnWindow {
    std::uint64_t start_ns{};
    std::uint64_t end_ns{};
    std::uint64_t phase_ns{};
    std::size_t gate{};
    bool open{};
};

template <std::size_t Entries>
struct TsnSchedule {
    static_assert(Entries > 0U,
                  "[ARC ERROR] arc::net::TsnSchedule needs gate storage. Action: set Entries to the fixed gate-list size.");

    std::uint64_t cycle_ns{};
    std::uint64_t base_ns{};
    std::array<TsnGate, Entries> gates{};
    std::size_t used{};

    [[nodiscard]] constexpr Status set(
        const std::size_t index,
        const TsnGate gate) noexcept
    {
        if (index >= Entries || !valid_gate(gate)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        gates[index] = gate;
        if (used < index + 1U) {
            used = index + 1U;
        }
        return ok();
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        if (cycle_ns == 0U || used == 0U || used > Entries) {
            return false;
        }
        for (std::size_t i = 0U; i < used; ++i) {
            if (!valid_gate(gates[i]) ||
                gates[i].offset_ns > cycle_ns ||
                gates[i].duration_ns > cycle_ns - gates[i].offset_ns) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr Result<TsnWindow> window(
        const std::uint64_t now_ns,
        const std::uint8_t traffic) const noexcept
    {
        if (!valid() || now_ns < base_ns) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto phase = (now_ns - base_ns) % cycle_ns;
        for (std::size_t i = 0U; i < used; ++i) {
            const auto& gate = gates[i];
            if (gate.traffic != traffic) {
                continue;
            }
            const auto start = gate.offset_ns + gate.guard_ns;
            const auto end = gate.offset_ns + gate.duration_ns - gate.guard_ns;
            if (phase >= start && phase < end) {
                return TsnWindow{
                    .start_ns = base_ns + ((now_ns - base_ns) / cycle_ns * cycle_ns) + start,
                    .end_ns = base_ns + ((now_ns - base_ns) / cycle_ns * cycle_ns) + end,
                    .phase_ns = phase,
                    .gate = i,
                    .open = true,
                };
            }
        }

        return fail(ESP_ERR_NOT_FOUND);
    }

    [[nodiscard]] constexpr bool allow(
        const std::uint64_t now_ns,
        const std::uint8_t traffic) const noexcept
    {
        return window(now_ns, traffic).has_value();
    }

    [[nodiscard]] constexpr Result<TsnWindow> window_local(
        const PtpClock& clock,
        const std::int64_t local_ns,
        const std::uint8_t traffic) const noexcept
    {
        const auto master = clock.to_master(local_ns);
        if (master < 0) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return window(static_cast<std::uint64_t>(master), traffic);
    }

    [[nodiscard]] constexpr bool allow_local(
        const PtpClock& clock,
        const std::int64_t local_ns,
        const std::uint8_t traffic) const noexcept
    {
        return window_local(clock, local_ns, traffic).has_value();
    }

    [[nodiscard]] constexpr Result<std::uint64_t> next_open(
        const std::uint64_t now_ns,
        const std::uint8_t traffic) const noexcept
    {
        if (!valid()) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        const auto elapsed = now_ns <= base_ns ? 0U : now_ns - base_ns;
        const auto cycle = elapsed / cycle_ns;
        const auto cycle_start = base_ns + (cycle * cycle_ns);
        auto best = std::numeric_limits<std::uint64_t>::max();
        for (std::size_t i = 0U; i < used; ++i) {
            const auto& gate = gates[i];
            if (gate.traffic != traffic) {
                continue;
            }
            const auto open_ns = gate.offset_ns + gate.guard_ns;
            if (cycle_start <= std::numeric_limits<std::uint64_t>::max() - open_ns) {
                const auto candidate = cycle_start + open_ns;
                if (candidate >= now_ns && candidate < best) {
                    best = candidate;
                }
            }
            if (cycle_start <= std::numeric_limits<std::uint64_t>::max() - cycle_ns) {
                const auto next_cycle = cycle_start + cycle_ns;
                if (next_cycle <= std::numeric_limits<std::uint64_t>::max() - open_ns) {
                    const auto wrapped = next_cycle + open_ns;
                    if (wrapped >= now_ns && wrapped < best) {
                        best = wrapped;
                    }
                }
            }
        }

        return best == std::numeric_limits<std::uint64_t>::max() ? Result<std::uint64_t>{fail(ESP_ERR_NOT_FOUND)}
                                                                 : ok(best);
    }

    [[nodiscard]] constexpr Result<std::uint64_t> next_local(
        const PtpClock& clock,
        const std::int64_t local_ns,
        const std::uint8_t traffic) const noexcept
    {
        const auto master = clock.to_master(local_ns);
        if (master < 0) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto next = next_open(static_cast<std::uint64_t>(master), traffic);
        if (!next) {
            return fail(next.error());
        }
        const auto local = clock.to_local(static_cast<std::int64_t>(*next));
        if (local < 0) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return static_cast<std::uint64_t>(local);
    }

private:
    [[nodiscard]] static constexpr bool valid_gate(const TsnGate gate) noexcept
    {
        return gate.duration_ns != 0U &&
            gate.guard_ns < gate.duration_ns &&
            gate.guard_ns < gate.duration_ns - gate.guard_ns;
    }
};

}  // namespace arc::net
