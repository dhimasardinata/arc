#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mapfile -t tests < <(find tools -maxdepth 1 -type f -name '*_test.py' | sort)
if ((${#tests[@]} == 0)); then
    echo "arc tool tests: no tests found" >&2
    exit 1
fi

jobs="${ARC_TOOL_TEST_JOBS:-}"
if [[ ! "$jobs" =~ ^[0-9]+$ || "$jobs" -le 0 ]]; then
    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
fi
if ((jobs > ${#tests[@]})); then
    jobs="${#tests[@]}"
fi
if ((jobs < 1)); then
    jobs=1
fi

tmp="$(mktemp -d)"
cleanup() {
    rm -rf "$tmp"
}
trap cleanup EXIT

go build -o "$tmp/arc-audit" tools/arc-audit.go
go build -o "$tmp/arc-gen" tools/arc-gen.go
export ARC_AUDIT_TEST_BIN="$tmp/arc-audit"
export ARC_GEN_TEST_BIN="$tmp/arc-gen"
export ARC_TOOL_TEST_TMP="$tmp"
printf '%s\0' "${tests[@]}" | xargs -0 -n1 -P "$jobs" bash -c '
    set -euo pipefail
    test="$1"
    log="$ARC_TOOL_TEST_TMP/$(basename "$test").log"
    if ! PYTHONDONTWRITEBYTECODE=1 python3 "$test" >"$log" 2>&1; then
        printf "arc tool test failed: %s\n" "$test" >&2
        cat "$log" >&2
        exit 1
    fi
' bash

echo "arc tool tests: OK (${#tests[@]} files, ${jobs} jobs)"
