#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ARC_HOST_BUILD_DIR:-$ROOT/build/host}"

CXX="${CXX:-g++}"

cmake -S "$ROOT/tests/host" -B "$BUILD" -G Ninja -DCMAKE_CXX_COMPILER="$CXX" >/dev/null
cmake --build "$BUILD" --target arc-host-logic

"$BUILD/arc-host-logic"
