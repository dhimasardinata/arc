#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "arc/rcu.hpp"
#include "arc/result.hpp"
#include "arc/tdma.hpp"
#include "arc/timesync.hpp"

namespace arc::net {

template <std::size_t Nodes>
struct SwarmSchedule {
    static_assert(Nodes > 0U, "swarm node count must be non-zero");

    std::array<std::uint32_t, Nodes> node_ids{};
    TdmaConfig tdma{};

    [[nodiscard]] constexpr std::uint32_t slot_for(const std::uint32_t node_id) const noexcept
    {
        for (std::uint32_t i = 0; i < Nodes; ++i) {
            if (node_ids[i] == node_id) {
                return i;
            }
        }
        return Nodes;
    }

    [[nodiscard]] constexpr TdmaWindow window(
        const std::uint32_t node_id,
        const std::uint64_t now_us) const noexcept
    {
        auto config = tdma;
        config.slot = slot_for(node_id);
        return Tdma::window(now_us, config);
    }

    [[nodiscard]] constexpr bool active(
        const std::uint32_t node_id,
        const std::uint64_t now_us) const noexcept
    {
        return window(node_id, now_us).active;
    }
};

template <RcuValue T>
struct DistributedRcuFrame {
    std::uint32_t node_id{};
    std::uint32_t sequence{};
    std::int64_t swarm_time_us{};
    T value{};
};

template <RcuValue T, std::size_t Nodes>
struct DistributedRcu {
    static_assert(Nodes > 0U, "distributed RCU needs at least one remote slot");

    using Frame = DistributedRcuFrame<T>;

    struct RemoteSlot {
        std::uint32_t node_id{};
        Rcu<T> state{};
        std::uint32_t sequence{};
        std::int64_t last_rx_us{};
        bool valid{};
    };

    std::uint32_t self{};
    std::uint32_t sequence{};
    Rcu<T> local{};
    std::array<RemoteSlot, Nodes> remotes{};

    constexpr explicit DistributedRcu(const std::uint32_t node_id = 0U) noexcept
        : self{node_id}
    {
    }

    [[nodiscard]] Status assign(
        const std::size_t index,
        const std::uint32_t node_id) noexcept
    {
        if (index >= Nodes || node_id == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        remotes[index].node_id = node_id;
        remotes[index].valid = false;
        remotes[index].sequence = 0U;
        remotes[index].last_rx_us = 0;
        return ok();
    }

    void write(const T& value) noexcept
    {
        local.write(value);
    }

    [[nodiscard]] Frame frame(const std::int64_t swarm_time_us) noexcept
    {
        return {
            .node_id = self,
            .sequence = ++sequence,
            .swarm_time_us = swarm_time_us,
            .value = local.read(),
        };
    }

    template <std::size_t ScheduleNodes>
    [[nodiscard]] Result<Frame> transmit(
        const SwarmSchedule<ScheduleNodes>& schedule,
        const TimeSync& clock,
        const std::int64_t local_now_us) noexcept
    {
        const auto swarm_now = clock.to_remote(local_now_us);
        if (swarm_now < 0) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (!schedule.active(self, static_cast<std::uint64_t>(swarm_now))) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return frame(swarm_now);
    }

    [[nodiscard]] Result<std::size_t> ingest(
        const Frame& next,
        const std::int64_t local_rx_us) noexcept
    {
        auto* const slot = find_slot(next.node_id);
        if (slot == nullptr) {
            return fail(ESP_ERR_NOT_FOUND);
        }
        if (slot->valid && next.sequence <= slot->sequence) {
            return fail(ESP_ERR_INVALID_STATE);
        }

        slot->node_id = next.node_id;
        slot->state.write(next.value);
        slot->sequence = next.sequence;
        slot->last_rx_us = local_rx_us;
        slot->valid = true;
        return static_cast<std::size_t>(slot - remotes.data());
    }

    template <std::size_t ScheduleNodes>
    [[nodiscard]] Result<std::size_t> ingest(
        const Frame& next,
        const SwarmSchedule<ScheduleNodes>& schedule,
        const std::int64_t local_rx_us) noexcept
    {
        if (next.swarm_time_us < 0) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        if (!schedule.active(next.node_id, static_cast<std::uint64_t>(next.swarm_time_us))) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        return ingest(next, local_rx_us);
    }

    [[nodiscard]] Result<const T*> read_remote(const std::uint32_t node_id) const noexcept
    {
        const auto* const slot = find(node_id);
        if (slot == nullptr || !slot->valid) {
            return fail(ESP_ERR_NOT_FOUND);
        }
        return &slot->state.read();
    }

    [[nodiscard]] bool stale(
        const std::uint32_t node_id,
        const std::int64_t now_us,
        const std::int64_t timeout_us) const noexcept
    {
        const auto* const slot = find(node_id);
        if (slot == nullptr || !slot->valid || timeout_us <= 0) {
            return true;
        }
        return TimeSync::sat_sub(now_us, slot->last_rx_us) > timeout_us;
    }

    [[nodiscard]] static std::span<const std::uint8_t> bytes(const Frame& frame) noexcept
    {
        return {
            reinterpret_cast<const std::uint8_t*>(&frame),
            sizeof(Frame),
        };
    }

    [[nodiscard]] static Result<Frame> parse(const std::span<const std::uint8_t> data) noexcept
    {
        if (data.size() != sizeof(Frame)) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        Frame out{};
        std::memcpy(&out, data.data(), sizeof(out));
        return out;
    }

private:
    [[nodiscard]] RemoteSlot* find(const std::uint32_t node_id) noexcept
    {
        for (auto& slot : remotes) {
            if (slot.node_id == node_id) {
                return &slot;
            }
        }
        return nullptr;
    }

    [[nodiscard]] const RemoteSlot* find(const std::uint32_t node_id) const noexcept
    {
        for (const auto& slot : remotes) {
            if (slot.node_id == node_id) {
                return &slot;
            }
        }
        return nullptr;
    }

    [[nodiscard]] RemoteSlot* find_slot(const std::uint32_t node_id) noexcept
    {
        if (auto* const slot = find(node_id); slot != nullptr) {
            return slot;
        }
        for (auto& slot : remotes) {
            if (slot.node_id == 0U) {
                return &slot;
            }
        }
        return nullptr;
    }
};

struct DeadReckoning {
    template <typename Filter, typename A, typename Q>
    [[nodiscard]] static bool predict_stale(
        Filter& filter,
        const std::int64_t now_us,
        const std::int64_t last_rx_us,
        const std::int64_t timeout_us,
        const A& a,
        const Q& q) noexcept
    {
        if (timeout_us <= 0 || TimeSync::sat_sub(now_us, last_rx_us) <= timeout_us) {
            return false;
        }
        filter.predict(a, q);
        return true;
    }
};

}  // namespace arc::net
