#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ARC_HOST_BUILD_DIR:-$ROOT/build/host}"

CXX="${CXX:-g++}"

cmake -S "$ROOT/tests/host" -B "$BUILD" -G Ninja -DCMAKE_CXX_COMPILER="$CXX" >/dev/null
cmake --build "$BUILD" --target arc-host-bench

BENCH_OUTPUT="$("$BUILD/arc-host-bench")"
CXX_VERSION="$("$CXX" --version | sed -n '1p')"
HOST_VERSION="$(uname -srmo 2>/dev/null || uname -a)"
OUTPUT="$(
    printf '== host benchmark context ==\n'
    printf 'compiler %s\n' "$CXX_VERSION"
    printf 'host     %s\n' "$HOST_VERSION"
    printf '\n%s\n' "$BENCH_OUTPUT"
)"
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
