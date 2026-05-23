# Glossary

Arc uses firmware terms deliberately. This glossary keeps those terms short in
the API docs while still making the meaning easy to check.

## Core Model

| Term | Meaning in Arc |
| --- | --- |
| Core 0 | The ESP32-S3 core that owns service work: Wi-Fi, storage, logs, OTA, HTTP, TLS, DNS, and other framework-heavy paths. |
| Core 1 | The ESP32-S3 core reserved for deterministic loops, bounded compute, hot GPIO, and handoff lanes. |
| Service work | Work that may block, allocate, log, touch flash, wait for radio/network state, or call broad ESP-IDF services. |
| Realtime lane | A code path whose timing and ownership must stay bounded enough to reason about before flashing hardware. |
| Hot path | The inner loop or ISR-adjacent path where hidden allocation, logging, blocking calls, or task migration would be a bug. |
| Plane | A named work surface, usually a pinned task or static owner, that separates service work from deterministic work. |

## Ownership

| Term | Meaning in Arc |
| --- | --- |
| Caller-owned | The caller supplies storage, lifetime, and retry policy. Arc operates on the buffer or handle without silently owning it. |
| Static owner | A type or object that visibly owns a peripheral, queue, task, or buffer for its whole usable lifetime. |
| Static loan | A zero-storage `arc::StaticLoan` that proves the object has static storage and carries readonly or mutable access plus an optional core owner in the type. `arc::LoanPack` adds compile-time conflict checks and query helpers for task contracts. |
| Claim token | A runtime guard that rejects competing aliases for the same hardware owner. |
| Topology | The declared board truth: pins, peripherals, fixed task shape, and ownership boundaries. |
| Board policy | Product-specific decisions that Arc does not guess, such as wiring, power sequencing, trust, calibration, and failsafe behavior. |
| Fail-fast path | A setup path that aborts when a declared board invariant is broken. |
| Recoverable path | A setup or runtime path that returns `esp_err_t`, `arc::Status`, or `arc::Result<T>` so the app can retry or degrade. |

## Memory And DMA

| Term | Meaning in Arc |
| --- | --- |
| DMA-capable | Memory that the ESP32-S3 DMA engine can access safely for a given peripheral. |
| Capability buffer | Arc-owned storage tagged for placement such as DMA, SIMD, RTC, internal RAM, or PSRAM. |
| PSRAM | External pseudo-static RAM. Useful for capacity, but cache and latency rules must stay visible. |
| Cache handoff | The explicit clean, invalidate, or ownership step needed before a CPU, DMA engine, or another core uses a buffer. |
| Cache line | The alignment unit Arc uses when whole-line handoff is required for safe DMA/cache ownership. |
| Coherent copy | A copy operation whose buffer ownership and completion ticket are tied together so data is not reused too early. |
| Descriptor ring | A fixed set of DMA descriptors that describe buffers or 2D rows for hardware-driven transfers. |

## Build And Target

| Term | Meaning in Arc |
| --- | --- |
| ESP-IDF | Espressif's native SDK and build system; Arc stays ESP-IDF-native instead of abstracting it away. |
| `ARC_TARGET` | Arc's target selector. It defaults to `esp32s3` and feeds `IDF_TARGET` through Arc's environment and CMake path. |
| Arc feature | A short CMake feature name passed to `arc_requires(...)`, such as `udp`, `spi`, `copy`, or `store`. |
| Profile | A grouped Arc entry point such as `core`, `memory`, `net_codecs`, `math`, `crypto`, `robotics`, or `sandbox`. |
| Public header validation | A compile-database check proving public headers still build through the real ESP-IDF component graph. |
| Module page | A generated page for one public header, with purpose, fit guidance, CMake feature, source landmarks, and proof path. |

## Evidence

| Term | Meaning in Arc |
| --- | --- |
| Proof path | The smallest command or artifact that actually supports the claim being made. |
| Proof pack | A fixed `arc::proof::Pack` of source-visible claims and a cycle budget that release tooling can keep beside a workload artifact. |
| Host check | A local compile/test check that runs without ESP32-S3 hardware. Useful for logic and contracts, not timing claims. |
| Firmware build | An ESP-IDF build for the root project or an example. It proves buildability, not board behavior. |
| Runtime proof | Captured serial, network, benchmark, or HIL evidence from the relevant board and fixture. |
| HIL | Hardware-in-the-loop. A documented physical fixture plus machine-checkable artifact for board behavior evidence. |
| Safety case | Arc's evidence map and non-claim boundary. It is not a product certification. |
| Benchmark surface | The implementation family printed in benchmark output, such as `arc`, `idf`, `arduino`, or `std`. |

## Networking And Protocols

| Term | Meaning in Arc |
| --- | --- |
| Radio owner | The Core 0 owner of Wi-Fi/netif/event-loop state shared by UDP, ESP-NOW, and other radio paths. |
| Codec | A parser or encoder for bytes such as MQTT, WebSocket, CoAP, HTTP, URI, or fixed binary records. |
| Transport plane | The task or owner that moves bytes over UDP, ESP-NOW, TCP, TLS, USB, or another hardware path. |
| Payload policy | Product code that decides what messages mean, how to authenticate them, and how to retry or reject them. |

## Security And Licensing

| Term | Meaning in Arc |
| --- | --- |
| Arc-covered code | Code in this repository that is licensed under Arc's public or commercial terms. |
| Public path | The `AGPL-3.0-only` route, with source-availability duties for Arc-covered code. |
| Commercial path | A paid signed agreement for named proprietary use. Cloning the repository does not create this grant. |
| Key custody | Product-specific rules for where keys come from, who can rotate them, and how provisioning is audited. Arc exposes helpers but does not own that policy. |

## Where To Go Next

- [Getting Started](/getting-started) for first build commands.
- [Safety Patterns](/safety-patterns) for copyable ownership and proof-contract shapes.
- [Production Integration](/production-integration) for product integration rules.
- [Troubleshooting](/troubleshooting) for symptom-first fixes.
- [Module Guide](/modules) for choosing the right header.
- [API Reference](/api) for exact public names and behavior.
