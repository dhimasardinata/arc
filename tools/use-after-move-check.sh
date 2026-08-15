#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "arc use-after-move check: SKIP (clang-tidy not found)"
    exit 0
fi

TIDY_ARGS=(
    -quiet
    "-checks=-*,bugprone-use-after-move"
    -warnings-as-errors=bugprone-use-after-move
)

COMPILE_ARGS=(
    -std=gnu++23
    -Itests/host/stubs
    -Icomponents/arc/include
    -pthread
)

stamp_dir="${ARC_HOST_BUILD_DIR:-$ROOT/build}/cache"
stamp_file="$stamp_dir/use-after-move.sum"

calc_hash() {
    {
        sha256sum tests/host/logic.cpp
        find components/arc/include -type f -name '*.hpp' -exec sha256sum {} +
    } 2>/dev/null | sha256sum | awk '{print $1}'
}

current_hash="$(calc_hash)"
if [[ -f "$stamp_file" && "$(<"$stamp_file")" == "$current_hash" ]]; then
    echo "arc use-after-move check: OK"
    exit 0
fi

tmp="$(mktemp -d)"
cleanup() {
    rm -rf "$tmp"
}
trap cleanup EXIT

probe="$tmp/std_expected_probe.cpp"
probe_log="$tmp/std_expected_probe.log"
cat >"$probe" <<'EOF'
#include <expected>

#include "arc/result.hpp"

int main()
{
    arc::Result<int> value{1};
    return value ? 0 : static_cast<int>(value.error());
}
EOF

if ! clang-tidy "${TIDY_ARGS[@]}" "$probe" -- "${COMPILE_ARGS[@]}" >"$probe_log" 2>&1; then
    if grep -Eq "invalid value 'gnu\\+\\+23'|no (template|member) named 'expected' in namespace 'std'|no member named 'unexpected' in namespace 'std'" "$probe_log"; then
        echo "arc use-after-move check: SKIP (clang-tidy cannot parse Arc C++23 std::expected headers)"
        exit 0
    fi
    cat "$probe_log" >&2
    echo "arc use-after-move check failed: clang-tidy C++23 probe failed" >&2
    exit 1
fi

clang-tidy \
    "${TIDY_ARGS[@]}" \
    tests/host/logic.cpp \
    -- \
    "${COMPILE_ARGS[@]}"

mkdir -p "$stamp_dir"
printf '%s' "$current_hash" > "$stamp_file"

echo "arc use-after-move check: OK"
