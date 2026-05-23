# Troubleshooting

Use this page when Arc fails before you know whether the problem is target
selection, CMake dependencies, ESP-IDF setup, generated docs, or evidence
scope. Start with the exact symptom, apply the smallest fix, then rerun the
command that failed.

## First Checks

Run these from the repository root unless the command says otherwise:

```sh
git status --short --branch
source ./env.sh
echo "$IDF_TARGET"
```

Expected target for normal work is `esp32s3`. If the shell cannot find
`idf.py`, reload the ESP-IDF environment through `source ./env.sh` before
debugging Arc itself.

## Fast Symptom Map

| Symptom | Likely cause | Fix | Proof |
| --- | --- | --- | --- |
| `IDF_TARGET` is not `esp32s3` | shell was not loaded through Arc's wrapper | run `source ./env.sh` in the current shell | `echo "$IDF_TARGET"` prints `esp32s3` |
| docs or scripts mention `idf.py set-target` | target bypasses Arc's target policy | export `ARC_TARGET=...` before `source ./env.sh` | repo check stays clean |
| header include fails in firmware build | missing Arc feature in component CMake | add the matching `arc_requires(...)` feature | the same `idf.py build` reaches compile/link |
| public header validation fails | CMake dependency map or include surface drifted | fix `cmake/arc-deps.cmake` or the header include | `tools/clangd-compile-commands.py --validate-arc-headers` passes |
| clangd shows stale ESP-IDF errors | compile database is old or target changed | regenerate with the clangd tool | editor diagnostics match the build graph |
| VitePress cannot find a new page | page is not linked or path is wrong | add the page to `docs/.vitepress/config.mts` or the relevant reading map | `npm run docs:build` renders pages |
| module page index misses a header | `docs/modules.md` does not list the public header | add the header purpose to the module guide | `python3 tools/docs-module-pages.py` succeeds |
| safety-case check rejects wording | docs overclaim certification or evidence | narrow the claim to Arc evidence and product responsibility | `tools/safety-case-check.py` passes |
| benchmark output is not publishable | report lacks target, command, SDK, or raw serial output | rerun and keep raw output plus context | benchmark page reporting rules are satisfied |
| pre-push hook takes longer than expected | repo policy checks are running | wait for the hook; inspect the first failing check if it exits nonzero | push completes or shows the exact gate |

## Build Failures

When `idf.py build` fails, do not start by changing source paths. Check the
component dependency first.

1. Find the header the failing file includes.
2. Open [Module Guide](/modules) or the matching module page.
3. Copy the CMake feature listed for that header.
4. Add that feature to the component's `arc_requires(...)` call.
5. Rerun the same build command.

Example:

```cmake
arc_requires(main_requires core udp gpio time)
```

If the build still fails, keep the first compiler or linker error. Later errors
are often fallout from the first missing include, Kconfig option, or component.

## Clangd And Editor Diagnostics

Arc editor diagnostics should follow the same ESP-IDF component graph as the
build. Regenerate the compile database when headers, CMake features, target
selection, or ESP-IDF checkout state changes.

```sh
./tools/clangd-compile-commands.py --validate-arc-headers -o /tmp/arc_compile_commands.json
```

If the validation fails, treat it as a real dependency or public-header issue
unless the error clearly points to a missing local ESP-IDF environment.

## Docs Site Failures

The docs build regenerates per-header module pages before VitePress renders the
site.

```sh
npm run docs:build
```

Normal VitePress/Rollup warnings about chunk size or third-party pure comments
are not Arc documentation failures when the build exits with code `0`.

Failures usually mean one of these:

- a new public header is missing from `docs/modules.md`;
- a Markdown link points to a page that does not exist;
- `docs/.vitepress/config.mts` points to the wrong route;
- generated module pages were edited by hand instead of changing
  `tools/docs-module-pages.py` or `docs/modules.md`.

Regenerate generated pages through `python3 tools/docs-module-pages.py`; do not
repair generated module pages one at a time.

## Repo Policy Failures

`./tools/check-repo.sh` is the broad policy gate. It includes repository checks
that are easy to confuse with firmware build failures:

- profile manifest drift;
- topology literal checks;
- structural TLA+ checks;
- moved-from-use checks when `clang-tidy` is available;
- safety-case evidence checks;
- realtime entry audit.

Fix the named policy first. Do not mask a policy failure by weakening the docs
or removing an evidence path.
Use `./tools/safety-case-check.py --format json` when a release or CI artifact
needs the claim list, evidence paths, required commands, and non-claims without
parsing the human status line.
Use `python3 tools/compile-fail-check.py --format report` when a reviewer needs
the grouped negative static-borrow and scoped-endpoint cases in plain text, or
`--format json` when a CI artifact needs the same structure.

## Benchmark And Hardware Claims

A successful build does not prove a benchmark or hardware claim.

Before publishing a runtime statement, keep:

- command used;
- Arc commit SHA;
- ESP-IDF release or commit;
- board and flash/PSRAM configuration;
- attached devices or loopback wiring;
- raw serial output;
- benchmark surface names such as `arc`, `idf`, `arduino`, or `std`.

If Arduino lanes are absent from `examples/esp32s3/bench`, verify that
`tools/ensure-frameworks.sh` has created the local ignored `arduino-esp32/`
checkout or set `ARC_ARDUINO_PATH` to a valid checkout before treating the lane
as unavailable.

## CI Log Triage

When CI firmware builds fail, inspect the dumped ESP-IDF log files in the job
output. Arc workflows print the matching `build/log/idf_py_stdout_output_*` and
`build/log/idf_py_stderr_output_*` content so the real compiler or linker error
is visible.

Use the same failing command locally only after confirming it is relevant to the
change. Docs-only changes usually need docs checks, not firmware rebuilds.

## When To Escalate

Escalate from local checks to firmware or hardware proof only when the claim
requires it:

- dependency or API surface: public-header validation;
- firmware behavior: root or example build;
- board behavior: flash, monitor, and captured logs;
- electrical behavior: documented fixture and HIL artifact;
- performance: raw benchmark output under the benchmark policy;
- safety wording: safety case plus product-level hazard analysis.

If the evidence does not cover the claim, narrow the claim or gather stronger
evidence.
