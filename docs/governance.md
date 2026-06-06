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

## Release Path

- `RELEASE.md` is the release checklist for scope, repository gates, firmware
  evidence, security/safety review, licensing, and publish records.
- `tools/release-evidence.py --format json --require-clean` emits the
  machine-readable release metadata snapshot before tagging or publishing a
  release artifact.
- `THIRD_PARTY_NOTICES.md` is the notice checklist for ESP-IDF,
  Arduino-ESP32, documentation tooling, CI dependencies, and bundled product
  dependencies.
- `THIRD_PARTY_MANIFEST.json` plus
  `tools/third-party-manifest-check.py --format json` keeps the repository's
  dependency notice boundary machine-checkable.

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
