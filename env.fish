set script_dir (cd (dirname (status -f)); and pwd)

set -l arc_target "esp32s3"
if set -q ARC_TARGET; and test -n "$ARC_TARGET"
    set arc_target (string lower -- "$ARC_TARGET")
end

switch "$arc_target"
    case esp32s3 esp32p4
    case esp32s31
        set -l s31_gate ""
        if set -q ARC_EXPERIMENTAL_ESP32S31
            set s31_gate "$ARC_EXPERIMENTAL_ESP32S31"
        end
        if not string match -r -q '^(1|ON|TRUE|YES|on|true|yes)$' -- "$s31_gate"
            echo "ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON before sourcing env.fish." >&2
            return 1
        end
    case '*'
        echo "Unsupported ARC_TARGET='$arc_target'. Supported targets: esp32s3, esp32p4, esp32s31." >&2
        return 1
end

if set -q ARC_IDF_PATH; and test -f "$ARC_IDF_PATH/export.fish"
    set -gx IDF_PATH "$ARC_IDF_PATH"
else if set -q IDF_PATH; and test -f "$IDF_PATH/export.fish"
    set -gx IDF_PATH "$IDF_PATH"
else if test -f "$script_dir/esp-idf/export.fish"
    set -gx IDF_PATH "$script_dir/esp-idf"
    if not set -q IDF_TOOLS_PATH
        if test -d "$HOME/.espressif"
            set -gx IDF_TOOLS_PATH "$HOME/.espressif"
        else
            set -gx IDF_TOOLS_PATH "$script_dir/.espressif"
        end
    end
else
    echo "ESP-IDF not found. Set IDF_PATH/ARC_IDF_PATH or clone esp-idf into $script_dir/esp-idf." >&2
    return 1
end

if test "$arc_target" = esp32s31
    set -l arc_s31_missing
    for arc_s31_path in \
        components/soc/esp32s31 \
        components/hal/esp32s31 \
        components/esp_rom/esp32s31 \
        components/esp_system/ld/esp32s31
        if not test -e "$IDF_PATH/$arc_s31_path"
            set arc_s31_missing $arc_s31_missing "$arc_s31_path"
        end
    end
    if not test -e "$IDF_PATH/tools/cmake/toolchain-esp32s31.cmake"
        set arc_s31_missing $arc_s31_missing "tools/cmake/toolchain-esp32s31.cmake"
    end
    if not test -f "$IDF_PATH/tools/idf_py_actions/constants.py"; or not grep -Eq "['\"]esp32s31['\"]" "$IDF_PATH/tools/idf_py_actions/constants.py"
        set arc_s31_missing $arc_s31_missing "tools/idf_py_actions/constants.py target registration"
    end
    if test (count $arc_s31_missing) -gt 0
        echo "ESP-IDF at $IDF_PATH does not include complete esp32s31 target metadata. Missing: $arc_s31_missing. Set ARC_IDF_PATH/IDF_PATH to a preview ESP-IDF that supports ESP32-S31." >&2
        return 1
    end
end

source "$IDF_PATH/export.fish"
set -gx IDF_TARGET "$arc_target"
