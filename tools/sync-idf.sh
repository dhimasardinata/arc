#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

usage() {
    cat <<'EOF'
Usage: ./tools/sync-idf.sh [--stash] [--install] [--no-submodules]

Sync a local ESP-IDF checkout to Arc's pinned IDF release from
.github/workflows/build.yml.

Options:
  --stash          Stash dirty esp-idf/ changes before checkout.
  --install        Run install.sh for ARC_IDF_TARGET after checkout.
  --no-submodules  Skip ESP-IDF submodule update.

Environment:
  ARC_IDF_PATH     ESP-IDF checkout path. Defaults to ./esp-idf.
  ARC_IDF_VERSION  Override workflow pin, for example v6.0.1.
  ARC_IDF_REF      Override workflow commit/tag ref.
  ARC_IDF_TARGET   Install target. Defaults to esp32s3.
EOF
}

workflow_value() {
    local key="$1"
    sed -nE "s/^[[:space:]]*${key}:[[:space:]]*\"?([^\"#]+)\"?[[:space:]]*$/\1/p" \
        .github/workflows/build.yml | head -n 1
}

stash_dirty=0
install_tools=0
update_submodules=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stash)
            stash_dirty=1
            ;;
        --install)
            install_tools=1
            ;;
        --no-submodules)
            update_submodules=0
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
    shift
done

idf_dir="${ARC_IDF_PATH:-$ROOT/esp-idf}"
idf_version="${ARC_IDF_VERSION:-$(workflow_value ARC_IDF_VERSION)}"
idf_ref="${ARC_IDF_REF:-$(workflow_value ARC_IDF_REF)}"
idf_target="${ARC_IDF_TARGET:-esp32s3}"
idf_url="${ARC_IDF_URL:-https://github.com/espressif/esp-idf.git}"

if [[ -z "$idf_version" || -z "$idf_ref" ]]; then
    echo "sync-idf: could not read ARC_IDF_VERSION/ARC_IDF_REF from workflow" >&2
    exit 1
fi

dirty() {
    [[ -n "$(git -C "$idf_dir" status --porcelain)" ]]
}

if [[ ! -d "$idf_dir/.git" ]]; then
    git clone --depth 1 --branch "$idf_version" "$idf_url" "$idf_dir"
else
    git -C "$idf_dir" remote set-url origin "$idf_url"
    if dirty; then
        if [[ "$stash_dirty" -eq 1 ]]; then
            git -C "$idf_dir" stash push -u -m "arc-sync-idf-before-${idf_version}"
            if dirty; then
                echo "sync-idf: $idf_dir is still dirty after stash; clean nested submodules first" >&2
                git -C "$idf_dir" status --short
                exit 1
            fi
        else
            echo "sync-idf: $idf_dir is dirty; rerun with --stash or clean it first" >&2
            git -C "$idf_dir" status --short
            exit 1
        fi
    fi
    git -C "$idf_dir" fetch --depth 1 origin "refs/tags/${idf_version}:refs/tags/${idf_version}"
fi

git -C "$idf_dir" checkout --detach "$idf_ref"

if [[ "$update_submodules" -eq 1 ]]; then
    git -C "$idf_dir" submodule update --init --recursive --depth 1
fi

if [[ "$install_tools" -eq 1 ]]; then
    "$idf_dir/install.sh" "$idf_target"
fi

printf 'sync-idf: %s %s\n' \
    "$(git -C "$idf_dir" rev-parse --short HEAD)" \
    "$(git -C "$idf_dir" describe --tags --always --dirty 2>/dev/null || true)"

if dirty; then
    echo "sync-idf: checkout is still dirty" >&2
    git -C "$idf_dir" status --short
    exit 1
fi
