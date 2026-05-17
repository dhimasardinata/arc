# Arc Commercial License Policy

Effective for this repository unless a separate written agreement says
otherwise.

This policy separates the public Arc path from the paid Arc commercial path.
Choose the correct path before Arc is built into a product, device image,
hosted service, SDK, customer deliverable, or redistributed binary.

## 1. Public Arc License

Arc is published in this repository under `AGPL-3.0-only`.

The public license grants broad rights to use, study, copy, modify, run, and
distribute Arc, subject to the terms of the GNU Affero General Public License
version 3 only. If you distribute Arc-covered firmware or run modified
network-accessible Arc-covered software, you must satisfy the corresponding
AGPLv3-only source and notice obligations.

The public path is for users who can carry those reciprocal obligations as part
of their normal product, service, or project workflow. It is not a no-cost
proprietary embedded license.

## 2. Paid Arc Commercial Path

The paid commercial path is for product teams that need rights different from
the public `AGPL-3.0-only` grant.

Commercial/proprietary rights are not granted by this file. They exist only
under a separate written commercial license agreement signed by the Arc
copyright holder and the licensee.

The paid agreement must define the licensed product or product family, hardware
targets, versions, affiliates, users, redistribution rights, source-handling
terms, support term, fees, and any commercial-only deliverables. Rights not
expressly granted in that agreement remain reserved.

## 3. No Gratis Commercial Exception

No free commercial exception is granted by this repository. Downloading,
cloning, forking, building, linking, embedding, or distributing Arc from this
repository does not create a commercial license.

If a company wants to distribute proprietary firmware, keep modified Arc source
closed, or avoid AGPL network/source obligations, it needs a separate paid
commercial license.

Starting development under the public repository does not create permission to
ship the same Arc-covered product under commercial terms later unless a signed
agreement expressly grants that right.

## 4. Arc Commercial Floor

Any paid commercial license for Arc must keep the Arc-covered portions visible,
auditable, and bounded to the signed scope. The commercial floor is:

- Arc copyright notices and license notices must stay intact.
- Recipients must be told that Arc is included.
- Modifications to Arc itself must be tracked and made available to the
  commercial licensee and any required auditors under the signed agreement.
- Users must not be blocked from replacing or inspecting the Arc-covered library
  portion where the product architecture permits that replacement.
- Patent, anti-circumvention, and installation-information protections must
  remain strong enough for the Arc-covered portion.
- The commercial license must not silently sublicense Arc as a permissive or
  public-domain dependency.
- The licensee must not remove or bypass a commercial entitlement, license-key,
  feature gate, or compliance mechanism if one is used for a paid Arc build.

## 5. Fees, Scope, And Currency

Commercial use under the paid path is conditioned on the licensee being current
on all fees and other obligations required by the signed agreement.

The signed agreement must define the licensed products, users, affiliates,
hardware targets, versions, support term, field of use, and any redistribution
or sublicensing rights. Rights not expressly granted in that agreement remain
reserved.

If the licensee stops being current on required fees or compliance duties, the
commercial rights are suspended or terminate as stated in the signed agreement.
Without an active commercial grant, the only repository license is
`AGPL-3.0-only`.

## 6. What The Paid License Can Add

A signed paid license can add proprietary distribution rights, private support,
private security handling, private hardware-integration terms, warranty terms,
or source-availability terms that are different from AGPLv3-only.

Those extra rights apply only to the parties, products, versions, and term
defined in the signed agreement.

Arc does not currently publish a public commercial license key system. If Arc
later ships license-key or enterprise-feature enforcement, that mechanism will
only verify the signed commercial scope; it will not replace the signed
agreement.

## 7. Third-Party Components

Arc builds on ESP-IDF and other upstream components selected through CMake.
Third-party components keep their own notices and license terms.

A paid Arc commercial agreement does not rewrite upstream licenses. Upstream
permissive licenses also do not convert Arc-covered code into a permissive
dependency or weaken the public `AGPL-3.0-only` path.

## 8. Trademarks And Branding

This repository does not grant trademark rights. Product names, logos, badges,
or compatibility claims using Arc branding require separate permission unless
fair-use or nominative-use rules apply.

## 9. No Legal Advice

This notice is a repository policy statement, not legal advice. Users should ask
their own counsel whether the AGPL public path or a paid commercial path fits
their product.

## 10. Default If There Is No Signed Agreement

If there is no signed commercial agreement, the only license grant from this
repository is `AGPL-3.0-only`.
