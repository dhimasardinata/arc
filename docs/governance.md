# Governance

Arc governance is intentionally small and file-backed. Use these files before
opening a pull request, publishing release evidence, handling a vulnerability,
or distributing Arc-derived artifacts.

## Contribution Path

- `CONTRIBUTING.md` defines setup, API naming, security routing, validation, and
  ESP32-S31 gating expectations.
- `.github/pull_request_template.md` keeps pull requests tied to behavior,
  hardware scope, SDK/config impact, validation, and release evidence.
- `.github/ISSUE_TEMPLATE/` separates source/tooling bugs from firmware runtime
  reports and routes vulnerability details away from public issues.
- `.github/CODEOWNERS` keeps critical areas reviewable by the maintainer.
- `.github/dependabot.yml` keeps GitHub Actions and docs dependency update
  lanes scheduled, grouped, and targeted at `main`.

## Release Path

- `RELEASE.md` is the release checklist for scope, repository gates, firmware
  evidence, security/safety review, licensing, and publish records.
- `tools/source-manifest.py --format json --require-clean` emits the tracked
  regular-file source hash manifest for release source archives and artifact
  bundles.
- `tools/firmware-manifest.py --format json --require-artifacts` emits
  repository-scoped SHA-256 evidence for firmware `.bin` and `.elf` outputs.
- `tools/firmware-evidence-check.py .arc-artifacts` verifies firmware manifest,
  firmware provenance subjects, SLSA predicate metadata, firmware evidence
  index, and binary hashes before the binary bundle is uploaded.
- `tools/release-evidence.py --format json --require-clean` emits the
  machine-readable release metadata snapshot, root/GitHub/docs policy file
  hashes, and repo-local executable tool, source command, and npm script
  reference checks with SHA-256 digests before tagging or publishing a release
  artifact.
- `tools/evidence-index.py --format json` records generated evidence file sizes,
  hashes, and JSON status, and rejects outside-repository paths, malformed JSON,
  `ok: false`, or non-empty `problems` payloads before an evidence bundle is
  published.
- `tools/evidence-bundle-check.py .arc-artifacts` verifies that the evidence
  index sizes, hashes, and JSON status records, source manifest inventory,
  source tree digest, provenance subjects, SLSA predicate metadata, artifact
  hashes, release command checks, timezone-aware provenance timestamps, and
  source repository/commit/branch identities agree before CI uploads the bundle.
- `tools/evidence-workflow-check.py --format json` verifies that the build
  workflow generates, indexes, signs, checks, and uploads the complete
  repository evidence contract and firmware artifact evidence.
- `tools/sbom.py --format json` emits SPDX 2.3 SBOM evidence from
  repository-scoped third-party and docs npm lock manifests.
- `tools/license-policy-check.py --format json` rejects outside-repository
  inputs plus missing or unapproved docs npm package license declarations before
  evidence is uploaded.
- `tools/provenance.py --format json <artifact...>` emits an in-toto Statement
  with SLSA provenance metadata, SHA-256 subjects, and evidence-toolchain
  byproduct hashes for evidence bundles.
- CI uploads `arc-evidence` with an evidence index, source, third-party,
  safety-case, release metadata, workflow action pin, workflow policy,
  evidence workflow, Dependabot policy, and npm lockfile JSON plus license
  policy, secret-scan, SPDX SBOM, and in-toto provenance evidence for every
  build. Firmware builds also upload the firmware artifact manifest, firmware
  provenance, and evidence index beside binaries. Evidence uploads must set
  `include-hidden-files: true` because the generated evidence lives under
  `.arc-artifacts/`.
- `THIRD_PARTY_NOTICES.md` is the notice checklist for ESP-IDF,
  Arduino-ESP32, documentation tooling, CI dependencies, and bundled product
  dependencies.
- `THIRD_PARTY_MANIFEST.json` plus
  `tools/third-party-manifest-check.py --format json` keeps the repository-scoped
  dependency notice boundary machine-checkable.
- `tools/workflow-pins-check.py --format json` keeps GitHub Actions supply-chain
  references immutable by requiring full commit SHA pins, version comments, and
  approved remote action sources plus bounded checkout settings.
- `tools/workflow-policy-check.py --format json` keeps the approved workflow
  set, trigger maps, permissions, concurrency, runner image pins, job timeouts,
  formatter pins, multiline Bash `set -euo pipefail` preambles, and shell
  expression/cache-write guards inside the repository's approved CI policy.
- `tools/dependabot-policy-check.py --format json` keeps Dependabot version
  updates scoped to reviewed GitHub Actions and docs npm dependency lanes.
- `tools/npm-lock-check.py --format json` keeps repository-scoped docs
  dependency manifests synchronized, backed by registry integrity hashes, and
  limited to reviewed install-script packages.
- `tools/license-policy-check.py --format json` keeps docs npm dependency
  licenses inside Arc's approved release policy.
- `tools/secret-scan-check.py --format json` rejects high-confidence private
  keys and service tokens in source-visible files before evidence is published.
- `tools/sbom.py --format json` gives releases a machine-readable SPDX bill of
  materials for Arc, external dependency groups, and locked docs packages.
- `tools/provenance.py --format json` ties repository-scoped published evidence
  subjects and evidence-toolchain byproducts back to the exact Arc commit and
  workflow or local build context.
- `tools/evidence-bundle-check.py .arc-artifacts` checks that the repository-local
  uploaded evidence bundle is internally coherent, including the source manifest
  file inventory, instead of only present.
- `tools/evidence-workflow-check.py --format json` checks that CI keeps the
  repository and firmware evidence generation-before-upload contracts complete.

## Security And License Path

- `SECURITY.md` is the GitHub-facing vulnerability disclosure policy.
- [Security Automation](/security) explains CodeQL, Dependabot, and the product
  security review that CI cannot prove.
- [Licensing](/licensing) explains the public AGPL path, paid commercial path,
  and third-party component boundaries.

## Maintainer Rule

When a release or review needs proof, use source-backed evidence instead of
summary claims. Link the exact commit, command output, firmware log, benchmark,
HIL artifact, or generated JSON report that covers the claim being made.
