# Production Integration

Use this page when Arc is moving from example firmware into a product tree,
board support package, or customer deliverable. It is the practical checklist
between "the example builds" and "this integration is evidence-backed."

## Integration Contract

Arc should enter a product as a visible firmware substrate, not as a hidden
folder of copied headers.

Keep these decisions explicit:

- target silicon and ESP-IDF version;
- board topology owner;
- Arc feature list in CMake;
- Core 0 service work versus Core 1 deterministic work;
- buffer placement and cache/DMA ownership;
- build, runtime, benchmark, and safety evidence level;
- license path and third-party notices.

If one of those items is unknown, keep the integration in prototype status.

## Recommended Shape

Start from the closest shipped example, then move only the required pieces into
the product app.

| Layer | Product owner | Arc surface |
| --- | --- | --- |
| Target selection | build system | `env.sh`, `cmake/arc-idf.cmake`, `ARC_TARGET` |
| Dependency mapping | component CMake | `cmake/arc-deps.cmake`, `arc_requires(...)` |
| Board topology | one board header or owner type | `arc::Pins`, `arc::Topology`, claim tokens |
| Hot loop | Core 1 program type | `arc::App`, `arc::Tight`, queues, snapshots |
| Services | Core 0 task or plane | Wi-Fi, storage, OTA, logs, protocol drain |
| Evidence | release checklist | host checks, builds, serial logs, HIL artifacts |

Avoid scattering pin maps, DMA buffer ownership, or radio ownership across many
files. A professional Arc integration has one obvious place to inspect each
hardware truth.

## CMake Pattern

Each component should request only the Arc features it includes.

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/arc-deps.cmake)

arc_requires(main_requires core gpio udp time)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)
```

Use the relative path that matches the component's location. In this repository
the root `main/` component uses `../cmake/arc-deps.cmake`; examples use
`../../../../cmake/arc-deps.cmake`.

Do not add broad ad hoc include paths to make a build pass. If a public Arc
header needs another ESP-IDF component, add or fix that mapping in
`cmake/arc-deps.cmake`.

## Target Policy

ESP32-S3 is the stable default.

- Use `source ./env.sh` before builds.
- Set `ARC_TARGET=esp32s3` only when a script or CI matrix needs to be explicit.
- Set `ARC_TARGET=esp32p4` explicitly for no-radio ESP32-P4 builds.
- Keep ESP32-S31 work gated behind `ARC_TARGET=esp32s31` and
  `ARC_EXPERIMENTAL_ESP32S31=ON`.
- Do not document or run `idf.py set-target` as the Arc target-selection path.

Target drift is a release blocker because Arc's public API is tied to the
silicon capability map.

## Board Topology

Declare physical truth once. Treat duplicate pins, shared peripheral aliases,
and competing runtime owners as integration bugs.

```cpp
struct Board {
    using pins = arc::Pins<2, 4, 5, 18, 19>;
};

static_assert(arc::Topology<Board>);
static_assert(Board::pins::has<18>());
static_assert(Board::pins::index<19>() != Board::pins::npos);
```

Run `./tools/topology-check.py path/to/board.hpp --format report` to see the
literal pins grouped by board declaration, or `--format dot` to emit a Graphviz
view for design reviews. Use `--format mermaid` when the graph should live
directly in Markdown, or `--format json` when CI should keep structured release
evidence. The checker understands plain integer literals, C++ base prefixes and
digit separators, and ESP-IDF tokens such as `GPIO_NUM_4` and `GPIO_NUM_NC`; add
`--strict-unresolved` when a release gate should fail on pin tokens it cannot
reduce.

Keep board policy near this declaration:

- which task owns Wi-Fi and storage;
- which core owns the deterministic loop;
- which buffers are DMA-capable or cache-line aligned;
- which external devices are required for runtime proof.

## Core Split

Core 0 owns work that can block, allocate, log, or call framework services.
Core 1 owns bounded deterministic work.

Good Arc integrations move data across the split through compact lanes:

- `arc::Spsc` when one producer feeds one consumer;
- `arc::Mpsc` or `arc::Fanin` when several producers feed one drain;
- `arc::SeqReg` or `arc::Rcu` when the newest snapshot matters more than every
  intermediate value;
- binary log or trace lanes when Core 1 must report without formatting text.

Do not let Core 1 wait on Wi-Fi, DNS, TLS, VFS, NVS, OTA, or formatted logging.

## Validation Ladder

Use the smallest proof that covers the change, then stop claiming beyond that
proof.

| Change type | Minimum evidence |
| --- | --- |
| docs only | docs build or markdown/link check, plus `git diff --check` |
| host-compilable logic | `./tools/host-tests.sh` |
| public headers or CMake dependencies | `./tools/clangd-compile-commands.py --validate-arc-headers` |
| repo policy or evidence maps | `./tools/check-repo.sh` |
| firmware behavior | root firmware build and the closest example build |
| hardware behavior | serial log, fixture notes, board revision, and attached-device state |
| benchmark claim | raw benchmark output with commit, SDK, board, and command |
| safety-relevant claim | safety-case evidence plus product-level hazard analysis |

Passing a host check is not hardware evidence. Passing a firmware build is not a
timing claim. Keep release notes precise about the proof level.

## Release Checklist

Before publishing an Arc-based firmware release, record:

- Arc commit SHA;
- ESP-IDF commit or release tag;
- target and board revision;
- `sdkconfig.defaults` changes;
- Arc features requested through `arc_requires(...)`;
- examples or firmware projects built;
- serial logs or HIL artifact paths for hardware claims;
- benchmark raw output for performance claims;
- AGPL or signed commercial license path;
- changed third-party component versions and notices.

This checklist belongs with the product release artifact, not only in a local
terminal scrollback.

## Documentation Duties

When integration changes a public Arc surface, update the docs in the same
change:

- add the header to [Module Guide](/modules) when a new public header appears;
- regenerate [Module Page Index](/module-pages) with
  `python3 tools/docs-module-pages.py`;
- update [API Reference](/api) when names, return types, ownership, or failure
  behavior change;
- update [Examples](/examples) when a firmware project becomes the best proof
  path;
- update [Benchmarking](/benchmarking) or [Safety Case](/safety-case) before
  making benchmark or safety claims.

The docs should state what is proven, what is assumed, and what remains product
responsibility.

## Exit Criteria

An Arc integration is ready to leave prototype status when:

1. the target path is explicit and does not use `idf.py set-target`;
2. CMake dependencies come from `arc_requires(...)`;
3. physical topology has one source of truth;
4. Core 1 does not own framework service work;
5. buffer ownership is visible at DMA/cache boundaries;
6. the validation ladder has evidence for the claims being made;
7. license and notice duties are decided before distribution.

If any item is missing, keep the integration marked as experimental.
