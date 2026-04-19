set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH is not set. Source env.sh/env.fish before configuring this project.")
endif()

set(_arc_requested_target "${IDF_TARGET}")
if(DEFINED _arc_requested_target AND
   NOT _arc_requested_target STREQUAL "" AND
   NOT _arc_requested_target STREQUAL "esp32s3")
    message(WARNING "Arc overrides requested target '${_arc_requested_target}' with 'esp32s3'.")
endif()

set(IDF_TARGET "esp32s3" CACHE STRING "Arc target" FORCE)
set(ENV{IDF_TARGET} "esp32s3")

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
