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
- `./tools/host-tests.sh` when library behavior changed
- `tools/clangd-compile-commands.py --validate-arc-headers` when public headers,
  umbrella includes, or component dependencies changed
- `npm run docs:build` when docs or docs-site configuration changed
- `./tools/third-party-manifest-check.py --format json` before relying on
  third-party notice coverage
- `./tools/release-evidence.py --format json --require-clean` before tagging or
  publishing a release artifact

## Firmware Evidence

For firmware-facing releases, capture:

- root `idf.py build` result;
- at least one relevant `idf.py -C examples/... build` result;
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
- Confirm whether the release uses the public AGPL path or a commercial license.
- Include third-party notices for ESP-IDF, Arduino-as-component, docs tooling, or
  any product-specific dependencies that ship with the release.

## Publish Record

Store the release note, validation commands, firmware logs, benchmark output,
HIL evidence, and known limitations together. Link the exact commit, tag, or
artifact bundle so future regressions can be traced back to source.
