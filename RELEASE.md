# Release Checklist

Arc releases should be evidence-backed. Do not treat a green CI run as enough
for firmware, security, safety, or licensing claims.

## Scope

- Name the release commit or tag.
- List changed public headers, examples, docs, tools, and CMake surfaces.
- State whether the release is host-only, ESP32-S3 firmware-facing, or includes
  explicitly gated ESP32-S31 work.
- Record ESP-IDF version and commit when firmware builds or runtime behavior are
  part of the release.

## Required Repository Gates

- `./tools/check-repo.sh`
- `./tools/format.sh --check`
- `./tools/tool-tests.sh`
- `./tools/profile-manifest-check.py`
- `./tools/topology-check.py --quiet`
- `python3 tools/compile-fail-check.py`
- `./tools/arc-prove.sh`
- `./tools/use-after-move-check.sh`
- `go run tools/arc-audit.go -root . -all`
- `./tools/host-tests.sh` when library behavior changed
- `tools/clangd-compile-commands.py --validate-arc-headers` when public headers,
  umbrella includes, or component dependencies changed
- `./tools/source-manifest.py --format json --require-clean` before publishing a
  release source archive or artifact bundle
- `./tools/firmware-manifest.py --format json --require-artifacts` after
  firmware builds and before publishing firmware binaries
- `./tools/firmware-evidence-check.py .arc-artifacts` after firmware manifest,
  provenance, and evidence index generation, before publishing firmware binaries
- `./tools/safety-case-check.py --format json` before publishing safety,
  hardware-risk, or release-process claims
- `./tools/workflow-pins-check.py --format json` before relying on GitHub
  Actions supply-chain pinning or the approved remote action allowlist
- `./tools/workflow-policy-check.py --format json` before relying on workflow
  set, trigger, permission, runner, timeout, formatter, or shell-expression
  policy
- `./tools/dependabot-policy-check.py --format json` before relying on
  Dependabot coverage for GitHub Actions and docs npm dependency updates
- `./tools/evidence-index.py --format json` over generated evidence files before
  publishing an evidence bundle; the index records each file size and hash, and
  rejects malformed or failed evidence JSON
- `./tools/evidence-bundle-check.py .arc-artifacts` after the evidence index and
  provenance files are generated, before uploading or publishing the bundle; the
  check verifies the source manifest inventory against the checked-out files
- `./tools/evidence-workflow-check.py --format json` before relying on CI
  repository evidence upload coverage
- `./tools/sbom.py --format json` before publishing a source archive, firmware
  image, docs site, or customer evidence bundle that needs SPDX 2.3 dependency
  inventory
- `./tools/npm-lock-check.py --format json` before relying on docs dependency
  lockfile integrity or install-script review
- `./tools/license-policy-check.py --format json` before publishing docs or
  evidence bundles that include npm dependency metadata
- `./tools/secret-scan-check.py --format json` before uploading release
  evidence or source-visible artifacts
- `./tools/provenance.py --format json <artifact...>` before publishing an
  evidence or firmware bundle that needs in-toto/SLSA provenance for hashed
  subjects and evidence-toolchain byproducts
- `npm run docs:build` when docs or docs-site configuration changed
- `./tools/third-party-manifest-check.py --format json` before relying on
  third-party notice coverage
- `./tools/release-evidence.py --format json --require-clean` before tagging or
  publishing a release artifact; its JSON records root, GitHub, and docs policy
  file hashes and checks that repo-local release commands still point to
  executable tools, checked source files, or declared npm scripts with matching
  SHA-256 digests

## Firmware Evidence

For firmware-facing releases, capture:

- root `idf.py build` result;
- at least one relevant `idf.py -C examples/... build` result;
- firmware artifact manifest with SHA-256 for each `.bin` and `.elf`;
- firmware provenance tying the firmware manifest to the release commit and
  evidence toolchain;
- firmware evidence check output proving the firmware manifest, provenance,
  SLSA predicate metadata, evidence index, and binary hashes agree;
- serial logs, benchmark output, HIL artifact, or reason runtime evidence is not
  applicable;
- target chip, board, partitions, sdkconfig defaults, and external fixtures.

If firmware builds are intentionally skipped, state the reason and the fallback
evidence that still covers the changed surface.

## Security And Safety Review

- Check `SECURITY.md` for any private disclosure or embargoed issue.
- Read `docs/security.md` before relying on CodeQL or Dependabot status.
- Run or review `./tools/safety-case-check.py` output when safety claims,
  hardware evidence, or release documentation changed.
- Verify key custody, secure boot, flash encryption, OTA signing, certificate
  bundles, and provisioning policy on the actual product when applicable.

## Licensing And Notices

- Read `docs/licensing.md`.
- Review `THIRD_PARTY_NOTICES.md`.
- Validate `THIRD_PARTY_MANIFEST.json` with
  `./tools/third-party-manifest-check.py --format json`.
- Validate docs dependency license policy with
  `./tools/license-policy-check.py --format json`.
- Generate SPDX 2.3 SBOM evidence with `./tools/sbom.py --format json`.
- Confirm whether the release uses the public AGPL path or a commercial license.
- Include third-party notices for ESP-IDF, Arduino-as-component, docs tooling, or
  any product-specific dependencies that ship with the release.

## Publish Record

Store the release note, validation commands, firmware logs, benchmark output,
HIL evidence, source manifest JSON, firmware artifact manifest JSON,
provenance JSON, and known limitations together. Link the exact commit, tag, or
artifact bundle so future regressions can be traced back to source.

CI uploads `arc-evidence` for every build with an evidence index, source
manifest, third-party manifest, safety-case JSON, release-evidence JSON,
workflow action pin evidence, workflow policy evidence, npm lockfile evidence,
evidence workflow contract evidence, Dependabot policy evidence, license policy
evidence, secret-scan evidence, SPDX SBOM evidence, and in-toto provenance
evidence. The repository bundle check validates the source manifest file
inventory, source tree digest, evidence index, and provenance before upload.
Firmware builds also upload `arc-binaries` with the firmware artifact manifest,
firmware provenance, and validated firmware evidence index beside the binaries.
