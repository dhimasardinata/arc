#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SPEC_DIR="${ARC_SPEC_DIR:-$ROOT/specs}"

required=(Spsc Consensus Roles)

for module in "${required[@]}"; do
    spec="$SPEC_DIR/$module.tla"
    [[ -f "$spec" ]] || {
        echo "arc-prove: missing $spec" >&2
        exit 1
    }
    grep -q -- "---- MODULE $module ----" "$spec" || {
        echo "arc-prove: $module has no module header" >&2
        exit 1
    }
    grep -q -- "Spec ==" "$spec" || {
        echo "arc-prove: $module has no Spec predicate" >&2
        exit 1
    }
    grep -q -- "====" "$spec" || {
        echo "arc-prove: $module has no closing marker" >&2
        exit 1
    }
done

if command -v apalache-mc >/dev/null 2>&1; then
    for module in "${required[@]}"; do
        apalache-mc check "$SPEC_DIR/$module.tla"
    done
elif [[ -n "${TLA_TOOLS_JAR:-}" && -f "${TLA_TOOLS_JAR:-}" ]]; then
    for module in "${required[@]}"; do
        java -cp "$TLA_TOOLS_JAR" tlc2.TLC -deadlock "$SPEC_DIR/$module.tla"
    done
else
    echo "arc-prove: structural TLA+ checks OK (set apalache-mc or TLA_TOOLS_JAR for model checking)"
fi
