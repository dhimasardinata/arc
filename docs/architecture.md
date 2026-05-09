# Arc Architecture Book

Arc exists to keep ESP32-S3 realtime work explicit. Core 0 owns framework duties:
Wi-Fi, storage, logs, HTTP, OTA, and slow control. Core 1 owns deterministic loops
where queue allocation, task migration, or hidden cache ownership would become a
bug.

## Core 1 Does Not Use FreeRTOS Queues

FreeRTOS queues are useful on Core 0, but they hide copying, wakeups, and scheduler
policy. Core 1 paths use `arc::Spsc`, `arc::Mpsc`, `arc::Fanin`, `arc::SeqReg`,
and `arc::Rcu` so the handoff shape is visible in the type and the hot loop can
bound the work it performs.

## Cache-Line Padding Is Intentional

`arc::Mpsc` pads cells to `arc::cache_line` so producers and the consumer do not
fight over the same cache line. `arc::DenseMpsc` exists for RAM density when that
trade-off is worth making explicitly.

## InitTxn Prevents Partial Driver Leaks

ESP-IDF driver bring-up often allocates hardware/software state across multiple
calls. `arc::InitTxn` and `arc::RefInitTxn` keep the state word busy until every
step succeeds. Destructors roll back failed setup; `pass()` publishes readiness.

## DMA And Cache Ownership

`arc::dmabuf`, `arc::simdbuf`, `arc::Cache`, and `arc::Copy` make memory placement
and cache handoff part of the call site. A buffer crossing DMA, PSRAM, or another
core should not rely on comments to describe ownership.

ML and DSP spans that feed `arc::simd` kernels should come from `arc::simdbuf`
or equivalent 16-byte-aligned storage when performance matters. Vision kernels
keep integer-safe defaults unless silicon data says otherwise; for example,
`arc::vision::StarTracker::isqrt` should be compared against an S3-targeted
`sqrt` lane in `examples/bench` before replacing the integer path.

## Silicon-Facing Policy

Arc public headers expose static, caller-owned algorithms and policy hooks. Board
code owns the dangerous physical details: pin wiring, power limits, ISR routing,
radio trust, and calibration.
