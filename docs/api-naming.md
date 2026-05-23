# Arc API Naming

Arc optimizes for typed, short call sites. Public names should make the type carry context instead of spelling every concept into one identifier.

Rules:

- Types use `PascalCase`: `CoreMap`, `PtpClock`, `DmaChain`.
- Public functions, fields, enum values, and variables use lowercase names with at most two snake-case words.
- Prefer a short type or namespace plus a short member over three-word snake case.
- Conversion APIs should use a verb or direction. Put the destination format in a type or namespace, then use a member such as `from_yuv422`.
- Examples must use the same short public names they are teaching.
- Example directories should be one lowercase word when possible, two words at most.
- All-caps CMake/Kconfig/preprocessor names may stay descriptive because they follow ESP-IDF convention.
- Legacy Arc aliases and compatibility surfaces are still Arc API; rename or remove them instead of exempting them.
- External SDK names may keep vendor spelling only at direct boundary sites, for example `esp_err_t`, `gpio_num_t`, `idf_component_register`, and ESP-IDF function calls. Arc wrappers, policy hooks, examples, and docs must use Arc naming.

Good:

```cpp
static_assert(arc::soc::p4 || arc::soc::s31);
arc::soc::has<arc::soc::Cap::ptp>;
arc::CoreMap<>::det;
```

Avoid spelling target, feature, role, and status context into one long lowercase identifier. Put that context into the type, namespace, or enum instead.
