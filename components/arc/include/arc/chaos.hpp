#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/postmortem.hpp"
#include "arc/rcu.hpp"
#include "arc/result.hpp"

namespace arc::chaos {

enum class Fault : std::uint8_t {
    bit_flip,
    i2c_stall,
    espnow_drop,
    tight_overrun,
};

struct Event {
    Fault fault{};
    std::uint32_t payload{};
    std::uint32_t aux{};
};

struct Config {
    std::uint32_t seed{0xA5A5'1234U};
    std::uint32_t i2c_stall_us{250U};
    std::uint32_t tight_overrun_cycles{1U};
};

struct State {
    std::uint32_t rng{};
    std::uint32_t packets{};
    std::uint32_t injected{};
};

struct Monkey {
    [[nodiscard]] static std::uint32_t next(State& state, const Config config = {}) noexcept
    {
        if (state.rng == 0U) {
            state.rng = config.seed == 0U ? 1U : config.seed;
        }
        auto x = state.rng;
        x ^= x << 13U;
        x ^= x >> 17U;
        x ^= x << 5U;
        state.rng = x == 0U ? 1U : x;
        return state.rng;
    }

    [[nodiscard]] static bool drop_espnow_33(State& state) noexcept
    {
        const auto packet = ++state.packets;
        return (packet % 3U) == 0U;
    }

    template <typename Policy>
    [[nodiscard]] static Status inject(
        State& state,
        const Fault fault,
        const Config config,
        const std::span<std::uint8_t> sram = {}) noexcept
    {
        const auto random = next(state, config);
        ++state.injected;
        switch (fault) {
            case Fault::bit_flip:
                if (sram.empty()) {
                    return Status{fail(ESP_ERR_INVALID_ARG)};
                }
                return status(Policy::flip_sram(
                    sram,
                    random % sram.size(),
                    static_cast<std::uint8_t>(1U << (random & 7U))));
            case Fault::i2c_stall:
                return status(Policy::stall_i2c(config.i2c_stall_us));
            case Fault::espnow_drop:
                return status(Policy::drop_espnow(drop_espnow_33(state)));
            case Fault::tight_overrun:
                return status(Policy::tight_overrun(config.tight_overrun_cycles));
            default:
                return Status{fail(ESP_ERR_INVALID_ARG)};
        }
    }

    template <typename Policy, typename Dump>
    [[nodiscard]] static Status tick(
        State& state,
        const Config config,
        const std::span<std::uint8_t> sram = {}) noexcept
    {
        const auto random = next(state, config);
        const auto fault = static_cast<Fault>(random & 0x3U);
        auto injected = inject<Policy>(state, fault, config, sram);
        record<Dump>({
            .fault = fault,
            .payload = state.injected,
            .aux = injected ? 0U : static_cast<std::uint32_t>(injected.error()),
        });
        return injected;
    }

    template <typename Dump>
    static void record(const Event event) noexcept
    {
        Dump::append(LogEvent{
            .tick = event.payload,
            .id = log_id("arc.chaos"),
            .payload = static_cast<std::uint32_t>(event.fault),
            .aux = event.aux,
        });
    }

    template <typename T>
    static void recover_rcu(Rcu<T>& lane, const T& value) noexcept
    {
        lane.write(value);
    }
};

}  // namespace arc::chaos
