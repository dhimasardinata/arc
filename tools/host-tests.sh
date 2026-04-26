#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT

CXX="${CXX:-g++}"

"$CXX" \
    -std=gnu++23 \
    -O2 \
    -g \
    -Wall \
    -Wextra \
    -Werror \
    -pthread \
    -I"$ROOT/tests/host/stubs" \
    -I"$ROOT/components/arc/include" \
    "$ROOT/tests/host/logic.cpp" \
    "$ROOT/tests/host/plane_compile.cpp" \
    -o "$BUILD/arc-host-logic"

"$BUILD/arc-host-logic"
