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
      text: Module Guide
      link: /modules
    - theme: alt
      text: API Reference
      link: /api
    - theme: alt
      text: Licensing
      link: /licensing

features:
  - title: Fixed Silicon Contract
    details: Arc targets ESP32-S3 on ESP-IDF and keeps topology, memory capability, and task placement visible in code.
  - title: Hot Path Discipline
    details: Queue, DSP, codec, DMA, and realtime lanes use caller-owned storage and explicit ownership transfer instead of hidden framework heap.
  - title: Methodical Docs
    details: The README and website carry per-header module pages, API reference, examples, benchmark policy, licensing, and safety evidence.
---
