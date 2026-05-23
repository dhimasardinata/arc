# `arc/detail/scoped_result.hpp`

Internal scoped-callback return-shape helpers.

## Fit

- Use it when you are maintaining Arc internals and need the small support type behind a public module.
- Do not start here when application firmware is choosing a public API; start from the owning public header instead.
- Verification focus: keep direct use inside Arc code and preserve the public module contract that depends on this helper.

## Arc Contract

- Header: `arc/detail/scoped_result.hpp`
- Module group: Detail Headers
- CMake feature: `internal`
- Closest example: `.`

This is Arc implementation support. Application firmware should enter through the public module that owns the behavior.

## CMake And Include

Application code should not include this detail header directly. Keep direct includes inside Arc internals, tests, or a public wrapper that preserves the public contract.

## Source Landmarks

Source landmarks: `IsReferenceWrapper`, `ReferenceWrapperResult`, `SpanResult`, `StringViewResult`, `NonOwningViewResult`, `PlainScopedResult`.

## Start From Zero

- Find the public Arc module that uses this helper.
- Change the public wrapper or test first, then adjust the detail helper.
- Keep the helper small enough that application code still has a public entry point.
- Run the docs generator and host checks after the internal contract changes.

## Owner Skeleton

The owner is the public Arc wrapper or test that reaches this helper. Do not add a
new application-facing dependency on `arc/detail/...`.

```cpp
// Inside Arc internals or a focused test only.
#include "arc/detail/scoped_result.hpp"
```

## Step-By-Step Check

1. Name the public header that depends on this helper.
2. Keep the helper hidden behind that public header.
3. Add or update the host test that exercises the public behavior.
4. Regenerate module pages so this detail page stays synchronized.
5. Run the repository checks before publishing.

## Build Or Example

Use host checks for this internal helper before any firmware build.

```sh
python3 tools/docs_module_pages_test.py
./tools/host-tests.sh
```

## Runtime Check

This page does not create a user-facing runtime surface. Runtime proof belongs to
the public module or example that owns the behavior using this helper.

## Next Reading

- [Module Guide](/modules)
- [API Reference](/api)
- [Examples](/examples)
