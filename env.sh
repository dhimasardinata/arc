#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

arc_target="${ARC_TARGET:-esp32s3}"
arc_target="${arc_target,,}"
case "${arc_target}" in
    esp32s3|esp32p4) ;;
    esp32s31)
        if [[ ! "${ARC_EXPERIMENTAL_ESP32S31:-}" =~ ^(1|ON|TRUE|YES|on|true|yes)$ ]]; then
            echo "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON before sourcing env.sh." >&2
            return 1 2>/dev/null || exit 1
        fi
        ;;
    *)
        echo "Unsupported ARC_TARGET='${arc_target}'. Supported targets: esp32s3, esp32p4, esp32s31." >&2
        return 1 2>/dev/null || exit 1
        ;;
esac

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

if [[ "${arc_target}" == "esp32s31" ]]; then
    arc_s31_missing=()
    for arc_s31_path in \
        "components/soc/esp32s31" \
        "components/hal/esp32s31" \
        "components/esp_rom/esp32s31" \
        "components/esp_system/ld/esp32s31"; do
        if [[ ! -e "${IDF_PATH}/${arc_s31_path}" ]]; then
            arc_s31_missing+=("${arc_s31_path}")
        fi
    done
    if [[ ! -e "${IDF_PATH}/tools/cmake/toolchain-esp32s31.cmake" ]]; then
        arc_s31_missing+=("tools/cmake/toolchain-esp32s31.cmake")
    fi
    if [[ ! -f "${IDF_PATH}/tools/idf_py_actions/constants.py" ]] \
        || ! grep -Eq "['\"]esp32s31['\"]" "${IDF_PATH}/tools/idf_py_actions/constants.py"; then
        arc_s31_missing+=("tools/idf_py_actions/constants.py target registration")
    fi
    if (( ${#arc_s31_missing[@]} > 0 )); then
        echo "ESP-IDF at ${IDF_PATH} does not include complete esp32s31 target metadata. Missing: ${arc_s31_missing[*]}. Set ARC_IDF_PATH/IDF_PATH to a preview ESP-IDF that supports ESP32-S31." >&2
        return 1 2>/dev/null || exit 1
    fi
fi

. "${IDF_PATH}/export.sh"
export IDF_TARGET="${arc_target}"
