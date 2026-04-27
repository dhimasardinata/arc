#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT

CXX="${CXX:-g++}"

"$CXX" \
    -std=gnu++23 \
    -O3 \
    -DNDEBUG \
    -Wall \
    -Wextra \
    -Werror \
    -pthread \
    -I"$ROOT/tests/host/stubs" \
    -I"$ROOT/components/arc/include" \
    "$ROOT/tests/host/bench.cpp" \
    -o "$BUILD/arc-host-bench"

OUTPUT="$("$BUILD/arc-host-bench")"
printf '%s\n' "$OUTPUT"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    {
        echo "## Host Benchmarks"
        echo
        echo '```text'
        printf '%s\n' "$OUTPUT"
        echo '```'
    } >>"$GITHUB_STEP_SUMMARY"
fi
