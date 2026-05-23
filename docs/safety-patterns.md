# Safety Patterns

This page is the short path from a blank file to Arc's compile-time safety
surface. It does not replace the safety case or product hazard analysis; it
shows the code shapes that make ownership and lifetime visible before runtime.
The core snippets here are compile-checked in
`tests/host/safety_patterns_compile.cpp`.

## What Arc Can Prove

Arc can make several local mistakes fail during C++ template checking:

- the wrong core touching a `CoreLocal` value;
- mutable access to const static state;
- a mutable static loan packed beside another loan to the same object;
- queues, fan-in lanes, and RPC lanes exposing the wrong role endpoint;
- public headers missing their ESP-IDF component dependency.

Arc does not prove whole-program C++ lifetime safety. Raw pointers, raw
`std::span` lifetimes, vendor driver handles, board wiring, and product
failsafes still need ordinary engineering review and runtime evidence.

## Smallest Safe Shape

Start by declaring long-lived state once. Give it a core owner in the type.

```cpp
#include "arc/borrow.hpp"
#include "arc/task.hpp"

struct ControlState {
    std::uint32_t tick{};
};

constinit ControlState control_state{};

using ControlCell = arc::StaticRef<&control_state, arc::Core::core1>;
using ControlRead = ControlCell::Read;
using ControlWrite = ControlCell::Write;

static_assert(ControlCell::can_write<arc::Core::core1>());
static_assert(!ControlCell::can_write<arc::Core::core0>());
static_assert(arc::StaticLoanType<ControlRead>);
static_assert(arc::StaticWritable<ControlCell, arc::Core::core1>);
static_assert(!arc::StaticWritable<ControlCell, arc::Core::core0>);
static_assert(arc::loans_ok<ControlRead, ControlRead>());
static_assert(!arc::loans_ok<ControlRead, ControlWrite>());
```

Use the loans at the owner boundary:

```cpp
void control_tick()
{
    const auto before = ControlCell::snapshot();
    ControlCell::set(ControlState{.tick = before.tick + 1U});
    arc::set<ControlCell>(ControlState{.tick = before.tick + 2U});
    ControlCell::with_write([](ControlState& state) {
        state.tick += 1U;
    });
}
```

That is the basic Arc pattern: name the owner, make access mode explicit, and
let the wrong core or wrong access mode disappear from the overload set. The
free `arc::with_read<...>` and `arc::with_write<...>` helpers remain available
when code prefers the helper at the call site instead of on the owner type. For
simple reads, `StaticRef::snapshot()` copies out a value without exposing a
borrowed reference. Scoped callbacks can return `void` or a copied value;
returning a reference, raw pointer, `std::reference_wrapper`, common non-owning
view such as `std::span` / `std::string_view`, or `StaticLoan` fails the build
so the borrow token cannot escape the callback.
`StaticRef::set(value)` and `arc::set<Ref>(value)` cover whole-value assignment
when a callback would only expose a mutable reference for one store.

## Shared Task Contract

When a task needs several static resources, name the expected loan set. Repeated
readonly loans are valid. A mutable loan cannot be grouped with another loan to
the same object.

```cpp
constinit ControlState telemetry_state{};

using TelemetryCell = arc::StaticRef<&telemetry_state, arc::Core::core1>;
using TelemetryRead = TelemetryCell::Read;

using TelemetryInputs = arc::StaticReads<ControlCell, TelemetryCell>;
using ControlStep = arc::StaticEdit<ControlCell, TelemetryCell>;
using OwnerTelemetryInputs = ControlCell::Reads<TelemetryCell>;
using OwnerControlStep = ControlCell::Edit<TelemetryCell>;
static_assert(TelemetryInputs::count == 2U);
static_assert(OwnerTelemetryInputs::count == 2U);
static_assert(OwnerControlStep::count == 2U);
static_assert(TelemetryInputs::contains<ControlRead>());
static_assert(OwnerTelemetryInputs::contains<ControlRead>());
static_assert(TelemetryInputs::reads<ControlCell>());
static_assert(TelemetryInputs::reads<TelemetryCell>());
static_assert(!TelemetryInputs::writes<ControlCell>());
static_assert(arc::HasStaticRefRead<TelemetryInputs, TelemetryCell>);
static_assert(ControlStep::contains<ControlWrite>());
static_assert(OwnerControlStep::contains<ControlWrite>());
static_assert(ControlStep::contains<TelemetryRead>());
static_assert(ControlStep::can_access<arc::Core::core1>());
static_assert(!ControlStep::can_access<arc::Core::core0>());
```

`StaticEdit<WriteRef, ReadRefs...>` is the common one-writer, many-reader shape.
If the task later tries to add the writer as one of its readers, `LoanPack`
fails the build. This is not full alias analysis; it is a deliberate contract
for the static objects listed in the task type.

Run code through the same contract when the callback should receive only the
references named by the loan pack:

```cpp
void control_step()
{
    arc::with_reads<ControlCell, TelemetryCell>([](const ControlState& control, const ControlState& telemetry) {
        static_cast<void>(control.tick + telemetry.tick);
    });
    ControlCell::with_reads<TelemetryCell>([](const ControlState& control, const ControlState& telemetry) {
        static_cast<void>(control.tick + telemetry.tick);
    });
    auto member_copies = ControlCell::snapshots<TelemetryCell>();
    static_cast<void>(member_copies);
    auto copies = arc::snapshots<ControlCell, TelemetryCell>();
    static_cast<void>(copies);
    ControlCell::with_edit<TelemetryCell>([](ControlState& control, const ControlState& telemetry) {
        control.tick += telemetry.tick;
    });
    arc::with_edit<ControlCell, TelemetryCell>([](ControlState& control, const ControlState& telemetry) {
        control.tick += telemetry.tick;
    });
}
```

## Cross-Core Handoff

Use `CoreLocal` and `CoreMsg` when the value itself should remember which core
may touch it.

```cpp
#include "arc/task.hpp"

using Core1Counter = arc::CoreLocal<std::uint32_t, arc::Core::core1>;
static_assert(arc::CoreLocalType<Core1Counter>);

Core1Counter counter{41U};
using CounterMsg = Core1Counter::Msg<arc::Core::core0>;
static_assert(arc::CoreMsgType<CounterMsg>);
static_assert(CounterMsg::from == arc::Core::core1);
static_assert(CounterMsg::to == arc::Core::core0);
counter.set<arc::Core::core1>(42U);
counter.set(42U);
auto current = counter.snapshot();
counter.with([](std::uint32_t& value) {
    value += 1U;
    return value;
});

auto msg = counter.msg<arc::Core::core0>();
static_assert(decltype(msg)::from == arc::Core::core1);
static_assert(decltype(msg)::to == arc::Core::core0);
static_cast<void>(msg.get());
static_cast<void>(msg.with([](const std::uint32_t& value) {
    return value;
}));
auto delivered = msg.snapshot();
```

For simple reads, `snapshot()` copies out a value instead of handing out a
reference. The message carries its destination in the type, so `msg.get()` reads
through that encoded destination, while `msg.with(fn)` scopes the read to one
callback. Explicit `get<Core>()` and `with<Core>(fn)` are still available when a
boundary should name the core directly. Like static-borrow helpers,
`CoreLocal::with` and `CoreMsg::with` callbacks cannot return references, raw
pointers, `std::reference_wrapper`, or common non-owning views such as
`std::span` / `std::string_view`. Explicit forms such as
`CoreLocal::with<Core>(fn)` and `msg<Core, To>()` remain available when a boundary
should spell the access core directly.

## Role Boundary

Use `Roles<Lane>` when setup code should hand out queue or RPC endpoints without
keeping the root lane API visible.

```cpp
#include "arc/roles.hpp"
#include "arc/spsc.hpp"

arc::Roles<arc::Spsc<std::uint32_t, 4>> events;
using EventProducer = decltype(events.producer());
using EventConsumer = decltype(events.consumer());

static_assert(arc::PushRole<EventProducer, std::uint32_t>);
static_assert(arc::PopRole<EventConsumer, std::uint32_t>);
static_assert(!arc::PopRole<EventProducer, std::uint32_t>);

void route_event()
{
    events.with_producer([](auto& producer) {
        return producer.try_push(7U);
    });
    events.with_consumer([](auto& consumer) {
        std::uint32_t event{};
        return consumer.try_pop(event);
    });
}
```

The `with_producer`, `with_consumer`, `with_split`, `with_client`, and
`with_server` helpers scope endpoint use to one callback. The callback may
return `void` or an ordinary copied value; returning an endpoint, reference, or
raw pointer fails the build. `PushRole`, `PopRole`, `RpcClientRole`, and
`RpcServerRole` let template boundaries accept only the endpoint side they
actually use. The scoped callback check rejects captured endpoint values too,
so a producer callback cannot smuggle out a consumer endpoint.

## Static Plane Launch

When a workload owns static state, bind the task to the same `StaticRef` instead
of repeating the state type and object address in separate places.

```cpp
#include "arc/plane.hpp"

struct ControlLoop {
    static void setup(ControlState&) {}
    static void run(ControlState&) noexcept {}
};

using ControlPlane = arc::StaticPlane<ControlLoop, ControlCell, 2048>;
static_assert(ControlPlane::core == arc::Core::core1);
static_assert(requires { ControlPlane::boot("control"); });
```

`StaticPlane` still boots through the same static-address contract as
`Plane<...>::boot<&state>()`; it only removes duplicated type plumbing.

## Proof Metadata

Attach small proof metadata to workloads when release tooling or review needs a
source-visible claim beside a measured cycle budget.

```cpp
#include "arc/proof.hpp"

using ControlProof = arc::proof::Pack<
    10'000U,
    arc::proof::Deadline<17U, 10'000U>,
    arc::proof::NoHeap<17U>,
    arc::proof::StaticLife<17U>>;

static_assert(ControlProof::has<arc::proof::Kind::deadline>());
static_assert(ControlProof::has<arc::proof::Kind::deadline, 17U>());
static_assert(ControlProof::bound<arc::proof::Kind::deadline>() == 10'000U);
static_assert(ControlProof::bound<arc::proof::Kind::deadline, 17U>() == 10'000U);
```

The proof pack is not a certificate. It is a compact artifact that keeps the
reviewed claim, subject id, and bound near the code that depends on it. Prefer
the subject-specific checks when a pack carries facts for more than one
workload.

## Verification Loop

Use host checks while shaping contracts:

```sh
./tools/host-tests.sh
./tools/format.sh --check
git diff --check
python3 tools/docs_module_pages_test.py
```

Use repo policy and public-header validation before publishing a safety-relevant
change:

```sh
./tools/check-repo.sh
./tools/clangd-compile-commands.py --validate-arc-headers -o /tmp/arc_compile_commands.json
```

For timing, wiring, control loops, RF, optics, motors, or actuators, host checks
are not enough. Build the closest firmware example and capture board or HIL
evidence that matches the claim.
