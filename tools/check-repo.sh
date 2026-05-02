#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

die() {
    echo "arc repo check failed: $*" >&2
    exit 1
}

git_files() {
    git ls-files "$@"
}

grep_files() {
    local pattern="$1"
    shift
    local files=("$@")
    [[ ${#files[@]} -gt 0 ]] || return 1
    grep -nE "$pattern" "${files[@]}"
}

git diff --check --cached >/dev/null 2>&1 || die "staged diff contains whitespace errors"

./tools/format.sh --check || die "format check failed; run ./tools/format.sh"

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "tracked sdkconfig or sdkconfig.old files are not allowed: $file"
    fi
done < <(git_files | grep -E '(^|/)sdkconfig(\.old)?$' || true)

while IFS= read -r file; do
    if [[ -e "$file" ]]; then
        die "sdkconfig.defaults.psram should not exist anymore: $file"
    fi
done < <(git_files | grep -E '(^|/)sdkconfig\.defaults\.psram$' || true)

if sed -n '/idf_component_register(/,/)/p' components/arc/CMakeLists.txt | grep -qw 'mbedtls'; then
    die "components/arc baseline must not require optional mbedTLS; use arc_requires(aes) or arc_requires(sha)"
fi

mapfile -t vscode_files < <(git_files .vscode || true)
for file in "${vscode_files[@]}"; do
    case "$file" in
        .vscode/settings.json|.vscode/extensions.json) ;;
        *) die ".vscode must only contain portable clangd settings: $file" ;;
    esac
done
if grep_files '/home/[^*]|/Users/[^*]|esp-[0-9]' "${vscode_files[@]}" >/dev/null 2>&1; then
    die ".vscode must not hardcode user-specific clangd toolchain paths"
fi

mapfile -t zed_files < <(git_files .zed || true)
for file in "${zed_files[@]}"; do
    case "$file" in
        .zed/settings.json) ;;
        *) die ".zed must only contain portable clangd settings: $file" ;;
    esac
done
if grep_files '/home/[^*]|/Users/[^*]|esp-[0-9]' "${zed_files[@]}" >/dev/null 2>&1; then
    die ".zed must not hardcode user-specific clangd toolchain paths"
fi

mapfile -t clangd_files < <(git_files | grep -E '(^|/)\.clangd$' || true)
for file in "${clangd_files[@]}"; do
    [[ "$file" == ".clangd" ]] || die "per-project .clangd files must stay generated/local: $file"
done
if grep_files 'MBEDTLS_CONFIG_FILE|TF_PSA_CRYPTO_USER_CONFIG_FILE|esp-idf/components/mbedtls|/\.espressif/|/home/|/Users/' "${clangd_files[@]}" >/dev/null 2>&1; then
    die ".clangd must not hardcode toolchain paths or optional mbedTLS include paths"
fi

mapfile -t docs < <(git_files README.md .github examples)
if grep_files '^[[:space:]]*idf\.py set-target ' "${docs[@]}" >/dev/null 2>&1; then
    die "docs or workflows still tell users to run set-target; Arc is locked to esp32s3"
fi

for file in CMakeLists.txt examples/*/CMakeLists.txt; do
    if ! grep -qE 'arc-idf\.cmake' "$file"; then
        die "$file does not include cmake/arc-idf.cmake"
    fi
done

if ! grep -qE 'set\(IDF_TARGET "esp32s3".*FORCE\)' cmake/arc-idf.cmake; then
    die "cmake/arc-idf.cmake no longer force-locks esp32s3"
fi

if ! grep -qE 'IDF_TARGET="esp32s3"|IDF_TARGET "esp32s3"' env.sh env.fish; then
    die "env loaders no longer export IDF_TARGET=esp32s3"
fi

if ! grep -qE 'for dir in \. examples/\*; do' .github/workflows/build.yml; then
    die "build workflow must auto-discover examples instead of hardcoding a partial list"
fi

if ! grep -qE '\./tools/host-tests\.sh' .github/workflows/build.yml; then
    die "build workflow must run host tests before firmware builds"
fi

if ! grep -qE '\./tools/host-bench\.sh' .github/workflows/build.yml; then
    die "build workflow must run host benchmarks before firmware builds"
fi

if ! grep -qE '\./tools/clangd-compile-commands\.py --validate-arc-headers' .github/workflows/build.yml; then
    die "build workflow must validate Arc header compile commands"
fi

for file in env.sh tools/host-tests.sh tools/host-bench.sh tools/framework-compare.sh tools/ensure-frameworks.sh tools/dump-source.py tools/clangd-compile-commands.py tools/format.sh tools/install-git-hooks.sh examples/*/env.sh; do
    [[ -x "$file" ]] || die "$file must stay executable"
done

echo "arc repo check: OK"
