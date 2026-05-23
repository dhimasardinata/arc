#include <cstdint>

#include "arc/borrow.hpp"
#include "arc/proof.hpp"
#include "arc/task.hpp"

namespace {

struct ControlState {
    std::uint32_t tick{};
};

constinit ControlState control_state{};
constinit ControlState telemetry_state{};

using ControlCell = arc::StaticRef<&control_state, arc::Core::core1>;
using ControlRead = decltype(ControlCell::read<arc::Core::core1>());
using ControlWrite = decltype(ControlCell::write<arc::Core::core1>());
using TelemetryCell = arc::StaticRef<&telemetry_state, arc::Core::core1>;
using TelemetryRead = decltype(TelemetryCell::read<arc::Core::core1>());
using TelemetryInputs = arc::LoanPack<ControlRead, TelemetryRead>;

static_assert(arc::StaticLoanType<ControlRead>);
static_assert(arc::loans_ok<ControlRead, ControlRead>());
static_assert(arc::loans_ok<ControlWrite, TelemetryRead>());
static_assert(!arc::loans_ok<ControlRead, ControlWrite>());
static_assert(TelemetryInputs::count == 2U);

using ControlProof = arc::proof::Pack<
    10'000U,
    arc::proof::Fact<arc::proof::Kind::deadline, 17U, 10'000U>,
    arc::proof::Fact<arc::proof::Kind::no_heap, 17U>,
    arc::proof::Fact<arc::proof::Kind::static_life, 17U>>;

static_assert(ControlProof::has<arc::proof::Kind::deadline>());
static_assert(ControlProof::bound<arc::proof::Kind::deadline>() == 10'000U);

[[maybe_unused]] void control_tick()
{
    auto state = ControlCell::write<arc::Core::core1>();
    state->tick += 1U;
}

[[maybe_unused]] void core_handoff()
{
    using Core1Counter = arc::CoreLocal<std::uint32_t, arc::Core::core1>;

    Core1Counter counter{41U};
    counter.set<arc::Core::core1>(42U);

    auto msg = counter.msg<arc::Core::core1, arc::Core::core0>();
    static_assert(decltype(msg)::from == arc::Core::core1);
    static_assert(decltype(msg)::to == arc::Core::core0);

    arc::CoreLocal<std::uint32_t, arc::Core::core0> mirror{};
    mirror.accept<arc::Core::core0>(msg);
    static_cast<void>(mirror.get<arc::Core::core0>());
}

}  // namespace
