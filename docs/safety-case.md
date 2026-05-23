# Arc Safety Case

This file is an engineering evidence map, not a functional-safety certificate.
Arc is not certified to MISRA C++:2023, ISO 26262, IEC 61508, DO-178C, or any
life-critical product standard. A product using Arc still needs its own hazard
analysis, process evidence, traceability, review records, tool qualification,
hardware validation, and certification authority acceptance.

## Scope

The current safety case only covers the Arc repository's ESP32-S3 firmware
substrate:

- public headers under `components/arc/include`
- CMake dependency mapping under `cmake/`
- host tests and static repo checks under `tools/`
- lightweight formal specs under `specs/`
- documented HIL shape in `docs/hil-test-jig.md`

Application policy, external wiring, sensors, actuators, enclosure safety,
network service policy, operator procedures, and any product-specific failsafe
remain outside Arc.

## Claims And Evidence

| Claim | Evidence |
| --- | --- |
| Target selection is explicit and cannot silently drift away from ESP32-S3. | `cmake/arc-idf.cmake`, `env.sh`, `env.fish`, `tools/check-repo.sh`, and CI guards reject missing target declarations and `idf.py set-target` docs. |
| Public headers compile under the actual ESP-IDF component graph. | `tools/clangd-compile-commands.py --validate-arc-headers` validates public Arc headers from generated compile databases. |
| Realtime loop entry points are kept away from known blocking, heap, queue send/receive/peek, task delay/suspend, timer-pend, task-notify wait, and logging calls. | `go run tools/arc-audit.go -root . -all` is run by `tools/check-repo.sh`. |
| Core-local state can carry its AMP owner and scoped callback leakage checks in the C++ type. | `arc::CoreLocal<T, Core>` exposes copy-out `snapshot()`, owner-inferred `set(value)` / `with(fn)` / `msg<To>()` / `accept(msg)` helpers, `CoreLocal::Msg<To>` / `Incoming<From>` message contracts, public `CoreLocalType` / `CoreMsgType` concepts, and explicit scoped `with<Core>(fn)` callbacks only to the owner core template argument, rejects reference/pointer/reference-wrapper/non-owning-view callback results, and `arc::CoreMsg<T, From, To>` exposes payload snapshots and scoped destination reads with the same callback-return checks, with compile-contract checks in `tests/host/logic.cpp` and negative cases in `tools/compile-fail-check.py`. |
| Static application state can carry static lifetime, access mode, optional core owner, same-contract alias checks, and scoped callback leakage checks in the C++ type. | `components/arc/include/arc/borrow.hpp` implements `arc::StaticRef`, `arc::StaticLoan`, copy-out `snapshot()` / `snapshots()` helpers, whole-value `set(value)`, `arc::StaticReads`, `arc::StaticWrites`, `arc::StaticEdit`, `arc::LoanPack`, and scoped `with_read`/`with_reads`/`with_write`/`with_edit` helpers over static objects, including `StaticRef::Reads<...>` / `StaticRef::Edit<...>` contracts plus member `StaticRef::snapshots<ReadRefs...>()`, `StaticRef::with_reads<ReadRefs...>(fn)`, and `StaticRef::with_edit<ReadRefs...>(fn)` for owner-anchored scopes, with callback checks rejecting reference/pointer/reference-wrapper/non-owning-view/static-loan returns, compile-contract checks in `tests/host/logic.cpp`, `tests/host/safety_patterns_compile.cpp`, and negative compile cases in `tools/compile-fail-check.py`. |
| Queue and fan-in ownership can be narrowed at setup boundaries. | `arc::Spsc`, `arc::Mpsc`, `arc::Fanin`, and their `arc::Audit<...>` wrappers expose producer/consumer role endpoints with host coverage in `tests/host/logic.cpp`. |
| Queue, fan-in, and RPC direct mutation can be removed from the static type surface. | `arc::Roles<Lane>` owns a lane privately, exposes only role endpoints such as producer/consumer or client/server, offers role endpoint concepts such as `PushRole`, `PopRole`, `RpcClientRole`, and `RpcServerRole`, and scopes role callbacks so captured endpoint/reference/pointer/reference-wrapper/non-owning-view returns fail, with compile-contract checks and host coverage in `tests/host/logic.cpp`, `tests/host/safety_patterns_compile.cpp`, and `tools/compile-fail-check.py`. |
| DMA descriptor binding rejects invalid topology before handoff. | `components/arc/include/arc/dma_chain.hpp` implements checked `arc::DmaChain::try_bind` for descriptor indexes, hardware-sized lengths, and optional `arc::CacheLines` coherent regions, with host coverage in `tests/host/logic.cpp`. |
| DMA descriptor buffers can be owned beside their descriptors. | `components/arc/include/arc/dma_chain.hpp` implements `arc::OwnedDmaChain`, which allocates DMA-capable `arc::CapsBuf` storage and binds descriptors to those owned buffers, with host coverage in `tests/host/logic.cpp`. |
| DMA/cache handoff avoids unaligned invalidation by default. | `components/arc/include/arc/cache.hpp` makes `arc::Cache::from_device` and `discard` require whole cache lines; `from_raw` and `discard_raw` are unavailable unless `ARC_ENABLE_UNSAFE_CACHE_RAW=1` and `arc::unsafe_raw` are used deliberately. |
| Cache-line handoff regions can be validated once and reused. | `components/arc/include/arc/cache.hpp` exposes `arc::Cache::lines` and `arc::CacheLines` for aligned whole-line regions, with host coverage in `tests/host/logic.cpp`. |
| DMA cache ownership can be carried in the C++ type when using Arc-owned buffers. | `components/arc/include/arc/cache.hpp` implements `arc::DmaBuf<T, arc::CacheState>`, whose CPU state exposes spans and whose device state only exposes handoff metadata until `from_device()` succeeds, with host coverage in `tests/host/logic.cpp`. |
| Async DMA copy completion can be bound to scope. | `arc::Copy::lease_coherent` returns move-only leases that finish the exact ticket on explicit `finish()` or destruction, with host coverage in `tests/host/logic.cpp`. |
| Async DMA buffer lifetime can be structurally owned for Arc-allocated DMA slabs. | `components/arc/include/arc/copy.hpp` implements `arc::Copy::lease_coherent(std::move(dst), std::move(src))`, which moves `arc::CapsBuf` storage into the active lease so the transfer owns both buffers until finish or destruction. |
| SDIO slave DMA queue and finish can be bound to one coherent buffer lifetime. | `components/arc/include/arc/sdio_slave.hpp` implements `arc::SdioSlave::lease_coherent`, which queues only strict cache-line buffers and invalidates them on finish, with host coverage in `tests/host/logic.cpp`. |
| Hardware crypto DMA completion can be bound to scope. | `components/arc/include/arc/crypto_dma.hpp` implements `arc::CryptoDma::lease`, with bounded-wait and explicit-finish host coverage in `tests/host/logic.cpp`. |
| Workloads can carry source-visible proof metadata beside a measured cycle budget. | `components/arc/include/arc/proof.hpp` implements `arc::proof::Pack`, `arc::proof::Fact`, common fact aliases, and subject-specific compile-time claim queries, with host coverage in `tests/host/logic.cpp`. |
| Formal-model files have at least structural coverage in every repo policy run. | `tools/check-repo.sh` runs `tools/arc-prove.sh`, which validates required TLA+ modules such as `specs/Spsc.tla`, `specs/Roles.tla`, and `specs/Consensus.tla`, checks the SPSC model head/tail names against `components/arc/include/arc/spsc.hpp`, and uses Apalache or TLC when available. |
| Moved-from handle misuse has static-analysis coverage on the host compile surface. | `tools/check-repo.sh` runs `tools/use-after-move-check.sh`, which invokes `clang-tidy` with `bugprone-use-after-move` as an error when the tool is available; intentional moved-from state probes in `tests/host/logic.cpp` must carry narrow `NOLINT` annotations. |
| HIL evidence has a machine-checkable artifact shape. | `docs/hil-test-jig.md` defines the required physical cases, and `tools/hil-evidence-check.py` validates captured JSONL artifacts before they are used as board evidence. |
| Safety claims stay tied to live repo evidence and non-claims. | `tools/check-repo.sh` runs `tools/safety-case-check.py`, which verifies claim evidence paths, required evidence commands, non-claim coverage, and certification-overclaim wording. |

## Required Local Evidence

Use [Safety Patterns](/safety-patterns) when writing the first local ownership
contract for a task or module. This safety case is the evidence map after the
contract exists.

Run these before treating a safety-relevant Arc change as reviewed:

```bash
./tools/check-repo.sh
./tools/host-tests.sh
./tools/arc-prove.sh
python3 tools/compile-fail-check.py
./tools/clangd-compile-commands.py --validate-arc-headers --changed-arc-headers HEAD -o /tmp/arc_compile_commands.json
```

For broad include or dependency changes, also run full public-header validation:

```bash
./tools/clangd-compile-commands.py --validate-arc-headers -o /tmp/arc_compile_commands.json
```

Firmware-facing changes still need the relevant ESP-IDF firmware build and, for
hardware behavior, board logs or HIL evidence. Passing host checks alone is not
evidence for electrical safety, timing margins on a real board, or product
hazard mitigation.

## Non-Claims

Arc does not claim:

- certification for aerospace, automotive, medical, industrial control, or other
  regulated life-critical deployment
- whole-program C++ lifetime safety equivalent to a borrow checker
- proof that application-owned `std::span` objects outlive every asynchronous
  hardware transaction
- safety of raw ESP-IDF driver interop adopted outside Arc's ownership wrappers
- safety of board wiring, external actuators, RF exposure, optics, ultrasonics,
  motor drives, or operator workflows
- security certification for cryptographic key lifecycle, provisioning policy,
  update provenance, or cloud/backend infrastructure

Any product that needs those claims must build a separate product safety case on
top of Arc and keep Arc's evidence as only one input.
