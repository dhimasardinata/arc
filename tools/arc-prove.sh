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

spsc_spec="$SPEC_DIR/Spsc.tla"
spsc_src="$ROOT/components/arc/include/arc/spsc.hpp"
grep -qF "head' = (head + 1) % Capacity" "$spsc_spec" || {
    echo "arc-prove: Spsc model push must advance head like arc::Spsc::try_push" >&2
    exit 1
}
grep -qF "tail' = (tail + 1) % Capacity" "$spsc_spec" || {
    echo "arc-prove: Spsc model pop must advance tail like arc::Spsc::try_pop" >&2
    exit 1
}
grep -qF "store_release(&head_, next);" "$spsc_src" || {
    echo "arc-prove: arc::Spsc::try_push no longer releases head_" >&2
    exit 1
}
grep -qF "store_release(&tail_, increment(tail));" "$spsc_src" || {
    echo "arc-prove: arc::Spsc::try_pop no longer releases tail_" >&2
    exit 1
}

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
