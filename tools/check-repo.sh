#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

die() {
    echo "arc repo check failed: $*" >&2
    exit 1
}

git diff --check --cached >/dev/null 2>&1 || die "staged diff contains whitespace errors"

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "tracked sdkconfig or sdkconfig.old files are not allowed: $file"
    fi
done < <(git ls-files | rg '(^|/)sdkconfig(\.old)?$')

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "sdkconfig.defaults.psram should not exist anymore: $file"
    fi
done < <(git ls-files | rg '(^|/)sdkconfig\.defaults\.psram$')

if rg -n '^\s*idf\.py set-target ' README.md examples .github --glob '!**/build/**' >/dev/null; then
    die "docs or workflows still tell users to run set-target; Arc is locked to esp32s3"
fi

for file in CMakeLists.txt examples/*/CMakeLists.txt; do
    if ! rg -q 'arc-idf\.cmake' "$file"; then
        die "$file does not include cmake/arc-idf.cmake"
    fi
done

if ! rg -q 'set\(IDF_TARGET "esp32s3".*FORCE\)' cmake/arc-idf.cmake; then
    die "cmake/arc-idf.cmake no longer force-locks esp32s3"
fi

if ! rg -q 'IDF_TARGET=\"esp32s3\"|IDF_TARGET "esp32s3"' env.sh env.fish; then
    die "env loaders no longer export IDF_TARGET=esp32s3"
fi

if ! rg -q 'for dir in \. examples/\*; do' .github/workflows/build.yml; then
    die "build workflow must auto-discover examples instead of hardcoding a partial list"
fi

for file in env.sh examples/*/env.sh; do
    [[ -x "$file" ]] || die "$file must stay executable"
done

echo "arc repo check: OK"
