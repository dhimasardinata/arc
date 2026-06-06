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
