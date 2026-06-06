# Third-Party Notices

Arc's own source in this repository is licensed through `LICENSE`, `COPYING`,
and `COMMERCIAL-LICENSE.md`. Third-party software keeps its own license terms.
This file is the maintainer-facing notice checklist for source releases and
product bundles; it does not replace product-specific legal review.
`THIRD_PARTY_MANIFEST.json` is the machine-checkable companion that lists the
repository's dependency groups, where they come from, and when notices must be
attached to a release artifact.

## Repository Source

Tracked Arc source does not vendor ESP-IDF, Arduino-ESP32, VitePress
dependencies, or GitHub Actions implementations. Local directories such as
`esp-idf/`, `arduino-esp32/`, `node_modules/`, and build outputs are ignored and
must not be treated as Arc-covered source.

## Build And Firmware Dependencies

- ESP-IDF: used as the firmware SDK and pinned by Arc build tooling. Preserve
  Espressif and upstream component notices when distributing firmware, SDK
  bundles, or source packages that include ESP-IDF material.
- Arduino-ESP32: optional benchmark/example integration. Preserve Arduino-ESP32
  and its upstream notices when shipping Arduino-as-component artifacts.
- Toolchains, Python packages, Go tools, and host OS packages: used for
  validation and build automation. Include their notices when they are bundled
  into a deliverable instead of being installed separately by the user or CI.

## Documentation And CI Dependencies

- VitePress and npm dependencies build the documentation site from
  `package.json` and `package-lock.json`. Keep npm package license notices when
  distributing the built site together with dependency sources or bundled
  runtime assets.
- GitHub Actions are referenced by pinned workflow entries under
  `.github/workflows/`. Their licenses and terms remain with each action
  provider.

## Product Release Rule

Before shipping a product, image, SDK, source archive, benchmark bundle, or docs
site derived from Arc:

1. List every third-party component actually included in the deliverable.
2. Include that component's license and notice text.
3. Keep Arc's AGPL or signed commercial-license notices separate from upstream
   notices.
4. Run `./tools/third-party-manifest-check.py --format json` and keep its
   output with the release evidence described by `RELEASE.md`.
