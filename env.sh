#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export IDF_PATH="${SCRIPT_DIR}/esp-idf"
export IDF_TOOLS_PATH="${SCRIPT_DIR}/.espressif"

. "${IDF_PATH}/export.sh"
