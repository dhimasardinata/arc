cmake_minimum_required(VERSION 3.16)

set(ARC_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../components")
set(ARC_SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/../../../sdkconfig.defaults"
    "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")

set(ARC_BENCH_HAS_ARDUINO OFF)
set(_arc_bench_arduino_dir "")
if(DEFINED ENV{ARC_ARDUINO_PATH} AND
   EXISTS "$ENV{ARC_ARDUINO_PATH}/CMakeLists.txt" AND
   EXISTS "$ENV{ARC_ARDUINO_PATH}/idf_component.yml")
    set(_arc_bench_arduino_dir "$ENV{ARC_ARDUINO_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../arduino-esp32/CMakeLists.txt" AND
       EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../arduino-esp32/idf_component.yml")
    set(_arc_bench_arduino_dir "${CMAKE_CURRENT_LIST_DIR}/../../../arduino-esp32")
endif()

if(NOT _arc_bench_arduino_dir STREQUAL "")
    list(APPEND ARC_COMPONENT_DIRS "${_arc_bench_arduino_dir}")
    list(APPEND ARC_SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.arduino")
    set(ARC_BENCH_HAS_ARDUINO ON)
    message(STATUS "arc_bench: enabling Arduino benchmark component from ${_arc_bench_arduino_dir}")
else()
    message(STATUS "arc_bench: Arduino benchmark component not found; firmware will compare Arc with raw ESP-IDF only")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/arc-idf.cmake)
arc_target(esp32s3)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(arc_bench)

# espressif/esp-modbus has an upstream strncpy bug that triggers
# -Werror=stringop-truncation on GCC 15+.  Demote the error back to a
# warning for that third-party target only; the warning is still visible.
if(ARC_BENCH_HAS_ARDUINO)
    idf_component_get_property(_arc_modbus_lib espressif__esp-modbus COMPONENT_LIB)
    if(TARGET ${_arc_modbus_lib})
        target_compile_options(${_arc_modbus_lib} PRIVATE -Wno-error=stringop-truncation)
    endif()
endif()
