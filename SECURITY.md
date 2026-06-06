# Security Policy

Arc is a firmware substrate, not a finished product. Treat this policy as the
repository disclosure path for Arc source, examples, docs, and GitHub
automation. Product vulnerabilities in a downstream device also need the
product owner's key, provisioning, OTA, backend, and hardware review.

## Supported Branches

| Branch | Security handling |
| --- | --- |
| `main` | Supported for source fixes, security automation updates, and docs corrections. |
| historical commits, forks, generated local builds | Not patched directly; rebase onto `main` before requesting a fix. |

Arc's ESP-IDF baseline is pinned in the build workflow and updated explicitly.
Do not assume a dependency bump is supported until it lands on `main`.

## Reporting

Do not open a public issue with exploit details, secrets, keys, certificates, or
device-specific credentials.

Use this order:

1. Use GitHub private vulnerability reporting if it is enabled for this
   repository.
2. If you already have a commercial or private maintainer channel for Arc, use
   that channel.
3. If no private channel is available, open a minimal public issue asking for a
   private security contact path. Do not include reproduction details there.

Include:

- affected Arc commit or tag;
- target chip and ESP-IDF version, if firmware behavior is involved;
- minimal reproduction steps or source path;
- expected impact and whether secrets, OTA, secure boot, flash encryption, or
  network trust are involved;
- whether the issue is already public anywhere else.

## Disclosure Handling

Arc maintainers should keep reports private until a fix, mitigation, or
non-affected determination is ready. Fixes should include the narrowest useful
source change plus matching evidence, such as host tests, repo policy checks,
firmware build logs, hardware logs, or docs updates.

Public disclosure should avoid publishing live secrets, production keys, or
unredacted device credentials. If a report affects product key custody,
provisioning, OTA signing, eFuse state, or backend infrastructure, that product
owner must own the final release decision.

## Security Automation

Repository security automation is documented in
[`docs/security.md`](docs/security.md). It covers CodeQL, Dependabot, and the
release-time human review that CI cannot prove.
