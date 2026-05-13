# Repository Guidelines

## Project Structure & Module Organization

Arc is an ESP-IDF firmware substrate for ESP32-S3 with experimental ESP32-S31 scaffolding. Core library headers live in `components/arc/include/arc/`, with `components/arc/include/arc.hpp` as the umbrella include. Firmware entry code lives in `main/app_main.cpp`. Shared CMake glue lives in `cmake/`, especially `arc-idf.cmake` and `arc-deps.cmake`. Examples are grouped by target under `examples/esp32s3/`, `examples/portable/`, and `examples/esp32s31/`, with their own `main/`, `CMakeLists.txt`, and optional `sdkconfig.defaults`. Host-only tests and stubs are in `tests/host/`. Repo tools are in `tools/`.

## Build, Test, and Development Commands

- `source ./env.sh`: load ESP-IDF and default `IDF_TARGET` to `esp32s3` unless `ARC_TARGET` is set.
- `idf.py build`: build the root firmware project.
- `idf.py -C examples/esp32s3/udp build`: build a specific ESP32-S3 example.
- `./tools/host-tests.sh`: compile and run host logic/compile tests with stubs.
- `./tools/host-bench.sh`: run host benchmarks and print compiler/host context.
- `./tools/format.sh --check`: verify C/C++ and Python formatting.
- `./tools/check-repo.sh`: run repository policy checks used by CI.
- `./tools/sync-idf.sh --stash`: sync local `esp-idf/` to Arc's pinned ESP-IDF release.
- `./tools/clangd-compile-commands.py --validate-arc-headers`: regenerate and validate `compile_commands.json` for IDEs.
- Zed and VS Code use checked-in clangd settings; keep them portable and backed by the repo `compile_commands.json`.

## Coding Style & Naming Conventions

Use C++ `gnu++26` for firmware and keep public APIs header-only unless existing patterns say otherwise. Format C/C++ with `clang-format`; format Python with `ruff format`. Prefer typed ownership, static storage, caller-owned buffers, explicit `esp_err_t` or `arc::Result<T>`, and no hidden heap use in hot paths. Headers use lowercase feature names, for example `arc/udp.hpp`, `arc/spi.hpp`, and `arc/cache.hpp`. Keep ESP-IDF component requirements in CMake, not ad hoc include paths.

Public Arc APIs should keep typing efficient: use `PascalCase` for types and lowercase names with at most two snake-case words for functions, fields, enum values, and variables. Prefer a short type or namespace plus a short member over long descriptive snake case, and keep examples symmetrical with the public names they teach. Legacy Arc compatibility names are not exempt; external SDK names keep vendor spelling only at direct boundary sites. See `docs/api-naming.md`.

## Testing Guidelines

Add host tests in `tests/host/logic.cpp` or small compile-contract files like `plane_compile.cpp`. Use `tests/host/stubs/` for ESP-IDF stand-ins. Run `./tools/host-tests.sh` before commits that affect library behavior. For firmware-facing changes, also build the root project and at least one relevant example. Validate public headers with `tools/clangd-compile-commands.py --validate-arc-headers` when includes or component dependencies change.

## Commit & Pull Request Guidelines

History uses short imperative subjects, for example `Fix clangd ESP-IDF include paths` and `Add DFS-safe timing and socket polling`. Keep commits focused. Pull requests should describe behavior changed, commands run, target hardware assumptions, and any SDK/config impact. Link issues when applicable and include serial logs or benchmark output for runtime or performance changes.

## Security & Configuration Tips

Do not commit `sdkconfig`, `sdkconfig.old`, toolchain paths, secrets, or user-specific `.clangd`, `.zed`, or `.vscode` settings. Keep ESP32-S3 as the stable default through `ARC_TARGET=esp32s3`; ESP32-S31 work must stay explicitly gated behind `ARC_TARGET=esp32s31` and `ARC_EXPERIMENTAL_ESP32S31=ON`. Do not add `idf.py set-target` instructions.
