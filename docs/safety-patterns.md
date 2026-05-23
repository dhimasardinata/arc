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
using ControlRead = decltype(ControlCell::read<arc::Core::core1>());
using ControlWrite = decltype(ControlCell::write<arc::Core::core1>());

static_assert(arc::StaticLoanType<ControlRead>);
static_assert(arc::loans_ok<ControlRead, ControlRead>());
static_assert(!arc::loans_ok<ControlRead, ControlWrite>());
```

Use the loans at the owner boundary:

```cpp
void control_tick()
{
    auto state = ControlCell::write<arc::Core::core1>();
    state->tick += 1U;
}
```

That is the basic Arc pattern: name the owner, make access mode explicit, and
let the wrong core or wrong access mode disappear from the overload set.

## Shared Task Contract

When a task needs several static resources, name the expected loan set. Repeated
readonly loans are valid. A mutable loan cannot be grouped with another loan to
the same object.

```cpp
constinit ControlState telemetry_state{};

using TelemetryCell = arc::StaticRef<&telemetry_state, arc::Core::core1>;
using TelemetryRead = decltype(TelemetryCell::read<arc::Core::core1>());

using TelemetryInputs = arc::LoanPack<ControlRead, TelemetryRead>;
static_assert(TelemetryInputs::count == 2U);
static_assert(TelemetryInputs::contains<ControlRead>());
static_assert(TelemetryInputs::reads<&control_state>());
static_assert(!TelemetryInputs::writes<&control_state>());
static_assert(arc::HasStaticRead<TelemetryInputs, &telemetry_state>);
```

If the task later tries to add `ControlWrite` beside `ControlRead`, `LoanPack`
fails the build. This is not full alias analysis; it is a deliberate contract
for the static objects listed in the task type.

## Cross-Core Handoff

Use `CoreLocal` and `CoreMsg` when the value itself should remember which core
may touch it.

```cpp
#include "arc/task.hpp"

using Core1Counter = arc::CoreLocal<std::uint32_t, arc::Core::core1>;

Core1Counter counter{41U};
counter.set<arc::Core::core1>(42U);

auto msg = counter.msg<arc::Core::core1, arc::Core::core0>();
static_assert(decltype(msg)::from == arc::Core::core1);
static_assert(decltype(msg)::to == arc::Core::core0);
```

The destination core can read the message. The source core cannot read it
through the destination-only accessor.

## Proof Metadata

Attach small proof metadata to workloads when release tooling or review needs a
source-visible claim beside a measured cycle budget.

```cpp
#include "arc/proof.hpp"

using ControlProof = arc::proof::Pack<
    10'000U,
    arc::proof::Fact<arc::proof::Kind::deadline, 17U, 10'000U>,
    arc::proof::Fact<arc::proof::Kind::no_heap, 17U>,
    arc::proof::Fact<arc::proof::Kind::static_life, 17U>>;

static_assert(ControlProof::has<arc::proof::Kind::deadline>());
static_assert(ControlProof::bound<arc::proof::Kind::deadline>() == 10'000U);
```

The proof pack is not a certificate. It is a compact artifact that keeps the
reviewed claim, subject id, and bound near the code that depends on it.

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
