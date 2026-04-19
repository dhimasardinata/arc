set script_dir (cd (dirname (status -f)); and pwd)

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

source "$IDF_PATH/export.fish"
set -gx IDF_TARGET "esp32s3"
