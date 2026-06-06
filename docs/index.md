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
      text: Integrate
      link: /production-integration
    - theme: alt
      text: Licensing
      link: /licensing
    - theme: alt
      text: Security
      link: /security

features:
  - title: Fixed Silicon Contract
    details: Arc targets ESP32-S3 on ESP-IDF and keeps topology, memory capability, and task placement visible in code.
  - title: Hot Path Discipline
    details: Queue, DSP, codec, DMA, and realtime lanes use caller-owned storage and explicit ownership transfer instead of hidden framework heap.
  - title: Methodical Docs
    details: The README and website carry per-header module pages, API reference, examples, benchmark policy, licensing, and safety evidence.
---

## Choose The Next Page

Use this map before opening the full API reference.

| If you need to... | Go here | Why |
| --- | --- | --- |
| build Arc for the first time | [Getting Started](/getting-started) | environment, first build, local checks, and the shortest reading path |
| understand why Arc splits Core 0 and Core 1 | [Architecture](/architecture) | ownership model, realtime limits, DMA/cache rules, and profile boundaries |
| write the first safe ownership contract | [Safety Patterns](/safety-patterns) | copyable static-lifetime, core-owner, loan-pack, and proof-pack patterns |
| move from example firmware to a product tree | [Production Integration](/production-integration) | CMake shape, target policy, board topology, validation ladder, and release evidence |
| diagnose a build, editor, docs, benchmark, or evidence failure | [Troubleshooting](/troubleshooting) | symptom-first fixes for target setup, CMake, clangd, VitePress, repo policy, and claims |
| decode Arc-specific words | [Glossary](/glossary) | short definitions for Core 0/Core 1, ownership, DMA/cache, target, evidence, protocol, and licensing terms |
| choose the right header | [Module Guide](/modules) | problem-first groups for every public Arc header |
| open one header page directly | [Module Page Index](/module-pages) | generated per-header pages with fit, CMake, source landmarks, and proof path |
| find exact public names and behavior | [API Reference](/api) | method-level notes mirrored from the source-backed README |
| pick firmware to build | [Examples](/examples) | one table for ESP32-S3 examples, portable examples, and ESP32-S31 scaffolds |
| follow contribution, review, and release policy | [Governance](/governance) | contribution path, PR/issue templates, CODEOWNERS, release evidence, and notice policy |
| understand repository security handling | [Security Automation](/security) | disclosure policy, CodeQL scanning, Dependabot update scope, and release-time human review duties |
| decide license path | [Licensing](/licensing) | Arc public AGPL path and paid commercial policy |
| publish performance claims | [Benchmarking](/benchmarking) | evidence levels and reporting rules |

## First Fifteen Minutes

1. Run `source ./env.sh`.
2. Build `idf.py -C examples/esp32s3/udp build`.
3. Read [Architecture](/architecture) to understand Core 0 service work versus Core 1 deterministic work.
4. Read [Safety Patterns](/safety-patterns) before writing shared state or cross-core handoff code.
5. Read [Production Integration](/production-integration) before moving code into a product tree.
6. Use [Module Guide](/modules) to choose the smallest header for your problem.
7. Read [Governance](/governance) before opening a pull request or preparing a release artifact.
8. Read [Security Automation](/security) and root `SECURITY.md` before relying on CI status for release evidence.
9. Open that header in [Module Page Index](/module-pages), then check exact names in [API Reference](/api).

## Documentation Shape

Arc docs are arranged from decision to evidence:

- start pages explain where to go next;
- safety patterns turn the ownership model into copyable code shapes;
- module pages map headers to purpose, dependency, and proof path;
- integration docs connect target policy, CMake shape, board topology, and evidence level;
- troubleshooting keeps common setup, build, editor, site, and evidence failures actionable;
- governance links contribution, review, ownership, release, security, and notice controls;
- security automation documents the disclosure policy, CodeQL, Dependabot, and the human release checks that CI cannot prove;
- the glossary keeps Arc vocabulary consistent across the README, site, and generated pages;
- examples show the buildable firmware surface;
- API pages keep exact names and ownership behavior available without reading the whole README;
- benchmarking, safety, and licensing pages state the rules before claims are published.
