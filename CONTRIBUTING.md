# Contributing

Arc changes should stay source-backed, reviewable, and easy to validate. This
guide is the short maintainer path for contributions; deeper project rules live
in `AGENTS.md`, `README.md`, and the docs site.

## Before You Change Code

1. Start from current `main`.
2. Run `source ./env.sh` so Arc keeps `esp32s3` as the default target.
3. Read `docs/api-naming.md` before adding or renaming public APIs.
4. Use `SECURITY.md` instead of public issues for vulnerabilities, secrets,
   keys, certificates, or exploit details.

Do not add `idf.py set-target` instructions. Arc target selection belongs in
`cmake/arc-idf.cmake`, `arc_target(...)`, and `ARC_TARGET`.

## Change Shape

- Keep public APIs header-only unless the surrounding Arc pattern requires
  something else.
- Prefer caller-owned buffers, explicit ownership transfer, typed result values,
  and static storage in hot paths.
- Keep ESP-IDF component requirements in CMake, not ad hoc include paths.
- Update docs, examples, safety evidence, or profile manifests when the behavior
  they describe changes.
- Keep ESP32-S31 work explicitly gated behind `ARC_TARGET=esp32s31` and
  `ARC_EXPERIMENTAL_ESP32S31=ON`.

## Validation

Run the narrowest checks that cover the change, and list them in the pull
request.

- Always run `./tools/check-repo.sh` before publishing maintainer changes.
- Run `./tools/host-tests.sh` when library behavior or host contracts changed.
- Run `tools/clangd-compile-commands.py --validate-arc-headers` when public
  headers, umbrella includes, or component dependencies changed.
- Run a relevant `idf.py build` for firmware-facing changes, or state why the
  build was intentionally skipped.
- Include serial logs, benchmark output, HIL artifacts, or safety-case evidence
  when claims depend on hardware or runtime behavior.

## Pull Requests

Use `.github/pull_request_template.md`. A reviewable PR should state:

- behavior changed;
- target hardware or host-only scope;
- SDK, CMake, or configuration impact;
- validation commands and results;
- release evidence or why it is not applicable.

CODEOWNERS routes critical areas to the maintainer. Keep commits focused and use
short imperative subjects.
