#include <cstdint>

#include "arc/borrow.hpp"
#include "arc/proof.hpp"
#include "arc/roles.hpp"
#include "arc/spsc.hpp"
#include "arc/task.hpp"

namespace {

struct ControlState {
    std::uint32_t tick{};
};

constinit ControlState control_state{};
constinit ControlState telemetry_state{};

using ControlCell = arc::StaticRef<&control_state, arc::Core::core1>;
using ControlRead = ControlCell::Read;
using ControlWrite = ControlCell::Write;
using TelemetryCell = arc::StaticRef<&telemetry_state, arc::Core::core1>;
using TelemetryRead = TelemetryCell::Read;
using TelemetryInputs = arc::StaticReads<ControlCell, TelemetryCell>;
using ControlStep = arc::StaticEdit<ControlCell, TelemetryCell>;
using OwnerTelemetryInputs = ControlCell::Reads<TelemetryCell>;
using OwnerControlStep = ControlCell::Edit<TelemetryCell>;

static_assert(ControlCell::can_write<arc::Core::core1>());
static_assert(!ControlCell::can_write<arc::Core::core0>());
static_assert(arc::StaticRefType<ControlCell>);
static_assert(arc::StaticLoanType<ControlRead>);
static_assert(ControlCell::can_read<arc::Core::core1>());
static_assert(!ControlCell::can_read<arc::Core::core0>());
static_assert(arc::StaticWritable<ControlCell, arc::Core::core1>);
static_assert(!arc::StaticWritable<ControlCell, arc::Core::core0>);
static_assert(arc::LoanWritable<ControlWrite, arc::Core::core1>);
static_assert(arc::loans_ok<ControlRead, ControlRead>());
static_assert(arc::loans_ok<ControlWrite, TelemetryRead>());
static_assert(!arc::loans_ok<ControlRead, ControlWrite>());
static_assert(TelemetryInputs::count == 2U);
static_assert(OwnerTelemetryInputs::count == 2U);
static_assert(OwnerControlStep::count == 2U);
static_assert(TelemetryInputs::contains<ControlRead>());
static_assert(OwnerTelemetryInputs::contains<ControlRead>());
static_assert(OwnerControlStep::contains<ControlWrite>());
static_assert(TelemetryInputs::reads<&control_state>());
static_assert(TelemetryInputs::reads<ControlCell>());
static_assert(TelemetryInputs::reads<&telemetry_state>());
static_assert(TelemetryInputs::reads<TelemetryCell>());
static_assert(!TelemetryInputs::writes<&control_state>());
static_assert(!TelemetryInputs::writes<ControlCell>());
static_assert(arc::HasLoan<TelemetryInputs, TelemetryRead>);
static_assert(ControlStep::contains<ControlWrite>());
static_assert(ControlStep::contains<TelemetryRead>());
static_assert(arc::HasStaticRead<TelemetryInputs, &control_state>);
static_assert(arc::HasStaticRefRead<TelemetryInputs, ControlCell>);
static_assert(!arc::HasStaticWrite<TelemetryInputs, &control_state>);
static_assert(!arc::HasStaticRefWrite<TelemetryInputs, ControlCell>);
static_assert(ControlStep::can_access<arc::Core::core1>());
static_assert(!ControlStep::can_access<arc::Core::core0>());
static_assert(ControlStep::writes<ControlCell>());
static_assert(ControlStep::reads<TelemetryCell>());

using ControlProof = arc::proof::Pack<
    10'000U,
    arc::proof::Deadline<17U, 10'000U>,
    arc::proof::NoHeap<17U>,
    arc::proof::StaticLife<17U>>;

static_assert(ControlProof::has<arc::proof::Kind::deadline>());
static_assert(ControlProof::has<arc::proof::Kind::deadline, 17U>());
static_assert(ControlProof::bound<arc::proof::Kind::deadline>() == 10'000U);
static_assert(ControlProof::bound<arc::proof::Kind::deadline, 17U>() == 10'000U);

[[maybe_unused]] void control_tick()
{
    const auto before = ControlCell::snapshot();
    ControlCell::set(ControlState{.tick = before.tick + 1U});
    arc::set<ControlCell>(ControlState{.tick = before.tick + 2U});
    ControlCell::with_write([](ControlState& state) {
        state.tick += 1U;
    });
}

[[maybe_unused]] void control_step()
{
    arc::with_reads<ControlCell, TelemetryCell>([](const ControlState& control, const ControlState& telemetry) {
        static_cast<void>(control.tick + telemetry.tick);
    });
    ControlCell::with_reads<TelemetryCell>([](const ControlState& control, const ControlState& telemetry) {
        static_cast<void>(control.tick + telemetry.tick);
    });
    auto member_copies = ControlCell::snapshots<TelemetryCell>();
    static_cast<void>(member_copies);
    ControlStep::with<arc::Core::core1>([](ControlState& control, const ControlState& telemetry) {
        control.tick += telemetry.tick;
    });
    ControlCell::with_edit<TelemetryCell>([](ControlState& control, const ControlState& telemetry) {
        control.tick += telemetry.tick;
    });
    arc::with_edit<ControlCell, TelemetryCell>([](ControlState& control, const ControlState& telemetry) {
        control.tick += telemetry.tick;
    });
}

[[maybe_unused]] void core_handoff()
{
    using Core1Counter = arc::CoreLocal<std::uint32_t, arc::Core::core1>;
    static_assert(arc::CoreLocalType<Core1Counter>);

    Core1Counter counter{41U};
    using CounterMsg = Core1Counter::Msg<arc::Core::core0>;
    static_assert(arc::CoreMsgType<CounterMsg>);
    static_assert(CounterMsg::from == arc::Core::core1);
    static_assert(CounterMsg::to == arc::Core::core0);
    counter.set<arc::Core::core1>(42U);
    counter.set(42U);
    counter.with([](std::uint32_t& value) {
        value += 1U;
    });
    static_cast<void>(counter.snapshot());

    auto msg = counter.msg<arc::Core::core0>();
    static_assert(decltype(msg)::from == arc::Core::core1);
    static_assert(decltype(msg)::to == arc::Core::core0);
    static_cast<void>(msg.get());
    static_cast<void>(msg.snapshot());
    static_cast<void>(msg.with([](const std::uint32_t& value) {
        return value;
    }));

    arc::CoreLocal<std::uint32_t, arc::Core::core0> mirror{};
    mirror.accept(msg);
    static_cast<void>(mirror.get<arc::Core::core0>());
}

[[maybe_unused]] void role_boundary()
{
    arc::Roles<arc::Spsc<std::uint32_t, 4>> events{};
    using EventProducer = decltype(events.producer());
    using EventConsumer = decltype(events.consumer());
    static_assert(arc::PushRole<EventProducer, std::uint32_t>);
    static_assert(arc::PopRole<EventConsumer, std::uint32_t>);
    static_assert(!arc::PopRole<EventProducer, std::uint32_t>);
    static_assert(!arc::PushRole<EventConsumer, std::uint32_t>);
    static_cast<void>(events.with_producer([](auto& producer) {
        return producer.try_push(7U);
    }));
    static_cast<void>(events.with_consumer([](auto& consumer) {
        std::uint32_t event{};
        return consumer.try_pop(event) && event == 7U;
    }));
}

}  // namespace
