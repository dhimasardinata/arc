#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

clone_or_update() {
    local url="$1"
    local dir="$2"
    local ref="${3:-}"

    if [[ -d "$dir/.git" ]]; then
        git -C "$dir" remote set-url origin "$url"
        git -C "$dir" fetch --depth 1 origin ${ref:+"$ref"}
    else
        rm -rf "$dir"
        if [[ -n "$ref" ]]; then
            git clone --depth 1 --branch "$ref" "$url" "$dir"
        else
            git clone --depth 1 "$url" "$dir"
        fi
    fi

    if [[ -n "$ref" ]]; then
        git -C "$dir" checkout --force FETCH_HEAD
    fi
}

if [[ ! -d esp-idf/.git ]]; then
    echo "local esp-idf/ not found; run ./tools/sync-idf.sh --install or set ARC_IDF_PATH for full firmware builds" >&2
else
    printf 'esp-idf      %s %s\n' \
        "$(git -C esp-idf rev-parse --short HEAD)" \
        "$(git -C esp-idf describe --tags --always --dirty 2>/dev/null || true)"
fi

clone_or_update \
    https://github.com/espressif/arduino-esp32.git \
    arduino-esp32 \
    "${ARC_ARDUINO_ESP32_REF:-}"

printf 'arduino-esp32 %s %s\n' \
    "$(git -C arduino-esp32 rev-parse --short HEAD)" \
    "$(git -C arduino-esp32 describe --tags --always --dirty 2>/dev/null || true)"
