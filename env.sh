#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -n "${ARC_IDF_PATH:-}" && -f "${ARC_IDF_PATH}/export.sh" ]]; then
    export IDF_PATH="${ARC_IDF_PATH}"
elif [[ -n "${IDF_PATH:-}" && -f "${IDF_PATH}/export.sh" ]]; then
    export IDF_PATH
elif [[ -f "${SCRIPT_DIR}/esp-idf/export.sh" ]]; then
    export IDF_PATH="${SCRIPT_DIR}/esp-idf"
    if [[ -z "${IDF_TOOLS_PATH:-}" ]]; then
        if [[ -d "${HOME}/.espressif" ]]; then
            export IDF_TOOLS_PATH="${HOME}/.espressif"
        else
            export IDF_TOOLS_PATH="${SCRIPT_DIR}/.espressif"
        fi
    fi
else
    echo "ESP-IDF not found. Set IDF_PATH/ARC_IDF_PATH or clone esp-idf into ${SCRIPT_DIR}/esp-idf." >&2
    return 1 2>/dev/null || exit 1
fi

. "${IDF_PATH}/export.sh"
export IDF_TARGET="esp32s3"
