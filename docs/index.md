---
layout: home

hero:
  name: Arc
  text: Deterministic ESP32-S3 firmware substrate
  tagline: Typed ownership, explicit DMA/cache boundaries, static Core 0/Core 1 composition, and benchmarked hot paths on ESP-IDF.
  actions:
    - theme: brand
      text: Start
      link: /getting-started
    - theme: alt
      text: Benchmark Policy
      link: /benchmarking

features:
  - title: Fixed Silicon Contract
    details: Arc targets ESP32-S3 on ESP-IDF and keeps topology, memory capability, and task placement visible in code.
  - title: Hot Path Discipline
    details: Queue, DSP, codec, DMA, and realtime lanes use caller-owned storage and explicit ownership transfer instead of hidden framework heap.
  - title: Evidence First
    details: CI runs repo policy checks, host tests, host benchmarks, formal structural checks, and selected firmware builds before changes land.
---
