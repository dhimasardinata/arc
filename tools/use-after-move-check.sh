#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "arc use-after-move check: SKIP (clang-tidy not found)"
    exit 0
fi

clang-tidy \
    -quiet \
    -checks=-*,bugprone-use-after-move \
    -warnings-as-errors=bugprone-use-after-move \
    tests/host/logic.cpp \
    -- \
    -std=gnu++23 \
    -Itests/host/stubs \
    -Icomponents/arc/include \
    -pthread

echo "arc use-after-move check: OK"
