set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH is not set. Source env.sh/env.fish before configuring this project.")
endif()

set(_arc_s31 OFF)
if(DEFINED ENV{ARC_EXPERIMENTAL_ESP32S31} AND
   "$ENV{ARC_EXPERIMENTAL_ESP32S31}" MATCHES "^(1|ON|TRUE|YES|on|true|yes)$")
    set(_arc_s31 ON)
endif()

if(NOT DEFINED ARC_EXPERIMENTAL_ESP32S31)
    option(ARC_EXPERIMENTAL_ESP32S31 "Enable experimental ESP32-S31 support" ${_arc_s31})
else()
    option(ARC_EXPERIMENTAL_ESP32S31 "Enable experimental ESP32-S31 support" ${ARC_EXPERIMENTAL_ESP32S31})
endif()

if((NOT DEFINED ARC_TARGET OR ARC_TARGET STREQUAL "") AND
   DEFINED ENV{ARC_TARGET} AND
   NOT "$ENV{ARC_TARGET}" STREQUAL "")
    set(ARC_TARGET "$ENV{ARC_TARGET}" CACHE STRING "Arc target" FORCE)
elseif(NOT DEFINED ARC_TARGET OR ARC_TARGET STREQUAL "")
    set(ARC_TARGET "esp32s3" CACHE STRING "Arc target" FORCE)
else()
    set(ARC_TARGET "${ARC_TARGET}" CACHE STRING "Arc target" FORCE)
endif()

string(TOLOWER "${ARC_TARGET}" ARC_TARGET)
set(ARC_TARGET "${ARC_TARGET}" CACHE STRING "Arc target: esp32s3, esp32p4, or esp32s31" FORCE)
set_property(CACHE ARC_TARGET PROPERTY STRINGS esp32s3 esp32p4 esp32s31)

set(_arc_req "")
if(DEFINED IDF_TARGET AND NOT IDF_TARGET STREQUAL "")
    set(_arc_req "${IDF_TARGET}")
elseif(DEFINED ENV{IDF_TARGET} AND NOT "$ENV{IDF_TARGET}" STREQUAL "")
    set(_arc_req "$ENV{IDF_TARGET}")
endif()

if(ARC_TARGET STREQUAL "esp32s3")
    set(_arc_idf "esp32s3")
elseif(ARC_TARGET STREQUAL "esp32p4")
    set(_arc_idf "esp32p4")
elseif(ARC_TARGET STREQUAL "esp32s31")
    if(NOT ARC_EXPERIMENTAL_ESP32S31)
        message(FATAL_ERROR "ARC_TARGET=esp32s31 requires -DARC_EXPERIMENTAL_ESP32S31=ON.")
    endif()
    set(_arc_idf "esp32s31")
else()
    message(FATAL_ERROR "Unsupported ARC_TARGET='${ARC_TARGET}'. Supported targets: esp32s3, esp32p4, esp32s31.")
endif()

if(NOT _arc_req STREQUAL "" AND
   NOT _arc_req STREQUAL _arc_idf)
    message(WARNING "Arc is configuring IDF_TARGET='${_arc_idf}' for ARC_TARGET='${ARC_TARGET}'; previous IDF_TARGET='${_arc_req}' is ignored.")
endif()

set(IDF_TARGET "${_arc_idf}" CACHE STRING "Arc ESP-IDF target" FORCE)
set(ENV{IDF_TARGET} "${_arc_idf}")

function(arc_target target)
    string(TOLOWER "${target}" _arc_need)
    if(NOT _arc_need STREQUAL "esp32s3" AND
       NOT _arc_need STREQUAL "esp32p4" AND
       NOT _arc_need STREQUAL "esp32s31")
        message(FATAL_ERROR "arc_target() supports esp32s3, esp32p4, or esp32s31, got '${target}'.")
    endif()

    if(NOT ARC_TARGET STREQUAL _arc_need)
        message(FATAL_ERROR "This Arc project requires ARC_TARGET=${_arc_need}; current ARC_TARGET=${ARC_TARGET}.")
    endif()
endfunction()

if(DEFINED ARC_COMPONENT_DIRS)
    set(EXTRA_COMPONENT_DIRS "${ARC_COMPONENT_DIRS}")
endif()

set(_arc_defaults)

if(DEFINED ARC_SDKCONFIG_DEFAULTS)
    foreach(_arc_default IN LISTS ARC_SDKCONFIG_DEFAULTS)
        if(EXISTS "${_arc_default}")
            list(APPEND _arc_defaults "${_arc_default}")
        endif()
    endforeach()
endif()

if(DEFINED SDKCONFIG_DEFAULTS AND NOT SDKCONFIG_DEFAULTS STREQUAL "")
    list(APPEND _arc_defaults ${SDKCONFIG_DEFAULTS})
endif()

if(_arc_defaults)
    list(REMOVE_DUPLICATES _arc_defaults)
    set(SDKCONFIG_DEFAULTS "${_arc_defaults}" CACHE STRING "Arc sdkconfig defaults" FORCE)
endif()
