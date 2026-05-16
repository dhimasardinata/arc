#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mode="write"
if [[ "${1:-}" == "--check" ]]; then
    mode="check"
    shift
fi
if [[ "${1:-}" == "--" ]]; then
    shift
fi

die() {
    echo "arc format failed: $*" >&2
    exit 1
}

need() {
    command -v "$1" >/dev/null 2>&1 || die "missing '$1'"
}

is_cpp() {
    case "$1" in
        *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx) return 0 ;;
        *) return 1 ;;
    esac
}

is_python() {
    case "$1" in
        *.py) return 0 ;;
        *) return 1 ;;
    esac
}

is_go() {
    case "$1" in
        *.go) return 0 ;;
        *) return 1 ;;
    esac
}

select_files() {
    if (($#)); then
        printf '%s\0' "$@"
    else
        git ls-files -z --cached --others --exclude-standard
    fi
}

ruff_cmd() {
    if command -v ruff >/dev/null 2>&1; then
        printf '%s\0' ruff
    elif python3 -m ruff --version >/dev/null 2>&1; then
        printf '%s\0' python3 -m ruff
    elif command -v uvx >/dev/null 2>&1; then
        printf '%s\0' uvx --from 'ruff>=0.8,<1' ruff
    elif command -v pipx >/dev/null 2>&1; then
        printf '%s\0' pipx run --spec 'ruff>=0.8,<1' ruff
    else
        die "missing 'ruff'; install ruff, uvx, or pipx"
    fi
}

cpp_files=()
python_files=()
go_files=()

while IFS= read -r -d '' file; do
    [[ -f "$file" ]] || continue
    if is_cpp "$file"; then
        cpp_files+=("$file")
    elif is_python "$file"; then
        python_files+=("$file")
    elif is_go "$file"; then
        go_files+=("$file")
    fi
done < <(select_files "$@")

if ((${#cpp_files[@]})); then
    need clang-format
    if [[ "$mode" == "check" ]]; then
        clang-format --dry-run --Werror "${cpp_files[@]}"
    else
        clang-format -i "${cpp_files[@]}"
    fi
fi

if ((${#python_files[@]})); then
    mapfile -d '' -t ruff < <(ruff_cmd)
    if [[ "$mode" == "check" ]]; then
        "${ruff[@]}" format --check "${python_files[@]}"
    else
        "${ruff[@]}" format "${python_files[@]}"
    fi
fi

if ((${#go_files[@]})); then
    need gofmt
    if [[ "$mode" == "check" ]]; then
        unformatted="$(gofmt -l "${go_files[@]}")"
        if [[ -n "$unformatted" ]]; then
            printf '%s\n' "$unformatted"
            die "Go files need gofmt"
        fi
    else
        gofmt -w "${go_files[@]}"
    fi
fi

echo "arc format: OK"
