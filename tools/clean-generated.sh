#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

find . \
    \( -name build -o -name 'build-*' -o -name managed_components \) \
    -prune -exec rm -rf {} +

find . \
    \( -name sdkconfig -o -name sdkconfig.old -o -name compile_commands.json \) \
    -type f -delete

echo "Removed generated build and sdkconfig state from $ROOT"
