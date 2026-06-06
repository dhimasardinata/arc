# Security Automation

Arc treats security checks as release evidence. The checks here do not replace
product threat modeling, hardware review, key management, or firmware runtime
testing, but they keep the repository's source and dependency surface visible on
every normal development path.

## Disclosure Policy

Root `SECURITY.md` is the GitHub-facing disclosure policy. It
states that `main` is the supported branch, asks reporters to avoid public
exploit details or credentials, and separates Arc repository fixes from final
product release decisions.

## CodeQL Code Scanning

`.github/workflows/codeql.yml` runs GitHub CodeQL on pushes to `main`, pull
requests, a weekly schedule, and manual dispatch.

The workflow scans:

- `c-cpp`, using `build-mode: none` so public headers, host C++ tests, and C/C++
  support code are analyzed without downloading ESP-IDF or building firmware;
- `python`, covering repo tools and documentation generators.

It enables the CodeQL `security-extended` and `security-and-quality` query
suites plus dependency caching. Firmware builds remain in the normal build
workflow, while CodeQL stays focused on source-level vulnerability and bug
patterns.

## Dependabot

`.github/dependabot.yml` enables weekly version-update pull requests for:

- GitHub Actions used by `.github/workflows/*.yml`;
- npm dependencies used by the docs site at the repository root.

The updates are grouped by ecosystem so workflow action bumps and docs
dependency bumps stay reviewable. Dependabot does not change ESP-IDF by itself;
Arc's pinned ESP-IDF version and commit stay controlled by `.github/workflows/build.yml`
and `tools/sync-idf.sh`.

`tools/workflow-pins-check.py --format json` backs that review path by rejecting
remote workflow actions that are not pinned to full commit SHA refs. Each remote
action line also keeps a trailing version comment so maintainers can review the
human release track without trusting a mutable tag. The same check requires
`actions/checkout` steps to set `persist-credentials: false`, because Arc
workflows do not need to leave the GitHub token in the local Git config after
checkout.

`tools/workflow-policy-check.py --format json` records the approved workflow
permission maps, rejects `pull_request_target`, requires concurrency controls,
pins every job to the reviewed Ubuntu runner image, requires every job to carry
`timeout-minutes`, rejects direct GitHub expression interpolation inside shell
scripts, and requires changed-base SHA values to be guarded as 40-hex commits
before shell steps pass them to `git`. It also keeps the CI Ruff formatter
install pinned to the same exact version as local fallback formatting. The only
job-level write permissions allowed by policy are `pages: write` and
`id-token: write` on the Pages deploy job.

`tools/npm-lock-check.py --format json` verifies that the docs `package-lock.json`
is npm lockfile v3, matches the root dependency declarations in `package.json`,
and keeps registry `resolved` URLs plus SHA-512 integrity strings for each
package entry. Transitive packages with install scripts must match Arc's
reviewed docs allowlist and exact reviewed versions, so a dependency update that
adds or changes install-time code fails before release evidence is uploaded.

`tools/license-policy-check.py --format json` validates the docs dependency
license declarations from `package-lock.json` against Arc's approved npm
license set. It fails missing, malformed, or unapproved license expressions
before CI uploads release evidence.

`tools/secret-scan-check.py --format json` scans tracked and non-ignored source
files for high-confidence private keys and service-token formats before CI
publishes release evidence. It is a leak guard, not a substitute for private
vulnerability reporting or product key custody review.

`tools/provenance.py --format json` emits in-toto/SLSA provenance over the
evidence subjects that CI publishes. It records the Arc commit, GitHub Actions
context when present, and SHA-256 digests for the evidence files so a release
review can trace an artifact bundle back to source without trusting artifact
names alone.

`tools/evidence-bundle-check.py .arc-artifacts` verifies that the evidence index
hashes, provenance subjects, SBOM commit, release metadata, and source manifest
all agree before CI uploads the repository evidence bundle.

`tools/evidence-workflow-check.py --format json` verifies that the build
workflow keeps every repository evidence file generated, included in provenance,
required by the index, checked as a bundle, and uploaded with explicit retention.

## What Still Needs Human Review

CodeQL and Dependabot are repository hygiene gates, not certification evidence.
Before a product release, still verify:

- key generation, storage, provisioning, and revocation policy;
- secure boot, flash encryption, and eFuse state on the actual target;
- network credentials and certificate bundles;
- OTA signing, rollback, and recovery behavior;
- third-party notices and the chosen Arc license path;
- hardware logs or HIL artifacts for claims that depend on a board.

See [Production Integration](/production-integration), [Safety Case](/safety-case),
and [Licensing](/licensing) before treating CI security status as release-ready
evidence.

Reference docs: [CodeQL for compiled languages](https://docs.github.com/en/code-security/concepts/code-scanning/codeql/about-codeql-code-scanning-for-compiled-languages),
[CodeQL build modes](https://docs.github.com/en/code-security/reference/code-scanning/codeql/codeql-build-options-and-steps-for-compiled-languages),
and [Dependabot version updates](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/configuring-dependabot-version-updates).
