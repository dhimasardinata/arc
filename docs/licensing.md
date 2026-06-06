# Licensing

Arc has two license paths, and neither path is implied by guesswork.

Choose the path before Arc becomes part of a product, device image, hosted
service, SDK, or customer deliverable. The public path and the paid commercial
path are separate choices for Arc-covered code; do not assume that starting on
one path creates permission to ship on the other.

## Public Arc Path

The public repository is licensed under `AGPL-3.0-only`.

Use this path when your firmware, service, or product workflow can satisfy the
AGPLv3-only source, notice, and network-use obligations for Arc-covered code.
If you distribute Arc-covered firmware or run modified network-accessible
Arc-covered software, keep the corresponding Arc-covered source available under
the same license terms.

The public path is suitable for projects that can carry reciprocal
source-availability duties as a normal part of distribution or hosted use. It is
not a quiet no-cost replacement for a proprietary embedded license.

## Paid Arc Commercial Path

The commercial path is paid, written, and explicit.

Use this path only when the Arc copyright holder and the licensee sign a
commercial agreement that names the licensed product, target hardware, scope,
term, redistribution rights, and fees. Proprietary firmware distribution,
private Arc modifications, private security handling, or terms that differ from
AGPLv3-only exist only inside that signed agreement.

Cloning, building, embedding, linking, or distributing Arc from this repository
does not create a commercial license.

Commercial eligibility also requires staying current on the fees, product
scope, support term, field of use, and compliance duties in the signed
agreement. If those conditions are not met, the repository grant falls back to
`AGPL-3.0-only`.

## Commercial Floor For Arc-Covered Code

A paid Arc license may relax the public AGPLv3-only path for the named product,
but it must not make Arc disappear into a silent permissive dependency. Arc's
commercial floor is:

- keep Arc notices intact;
- identify Arc in distributed products;
- track changes to Arc itself;
- preserve inspection or replacement rights for Arc-covered portions where the
  product architecture permits it;
- keep patent, anti-circumvention, and installation-information protections
  strong enough for the Arc-covered portion;
- keep rights limited to the products, targets, versions, and term named in the
  paid agreement.

## Product And Feature Scope

A paid Arc agreement can cover proprietary firmware distribution, private
source handling, private hardware-integration support, private security
handling, warranty terms, or commercial-only deliverables. Those rights are
bounded to the products, targets, versions, affiliates, users, and dates named
in the signed agreement.

Arc does not currently ship a public commercial license key system. Any future
license-key, entitlement, or enterprise-feature gate would be an enforcement
mechanism for the signed commercial scope, not a separate permission by itself.

## Third-Party Components

Arc builds on ESP-IDF and other upstream components selected through CMake.
Those third-party components keep their own license terms. A paid Arc agreement
does not rewrite upstream terms, and upstream permissive terms do not weaken
Arc's AGPLv3-only public path for Arc-covered code.

Use `THIRD_PARTY_NOTICES.md` as the repository notice checklist before
shipping a source archive, firmware image, SDK bundle, benchmark artifact, or
documentation site that includes upstream material.
Use `THIRD_PARTY_MANIFEST.json` and
`./tools/third-party-manifest-check.py --format json` when the release record
needs a machine-readable dependency notice boundary.
Use `./tools/sbom.py --format json` when the release record also needs SPDX 2.3
SBOM evidence for Arc, external dependency groups, and locked docs npm
packages.

The commercial license text in this repository is a policy statement, not an
automatic grant. The signed agreement controls the actual commercial rights.

## Decision Rule

Choose the public path when AGPLv3-only works for your product.

Choose the paid path before proprietary distribution, private Arc modification
handling, or any deployment that cannot meet AGPLv3-only.

See [COMMERCIAL-LICENSE.md](https://github.com/dhimasardinata/arc/blob/main/COMMERCIAL-LICENSE.md)
for the repository-level commercial license path.
