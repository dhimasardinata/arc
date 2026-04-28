cmake_minimum_required(VERSION 3.16)

set(ARC_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../components")
set(ARC_SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/../../sdkconfig.defaults"
    "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")

set(ARC_BENCH_HAS_ARDUINO OFF)
set(_arc_bench_arduino_dir "")
if(DEFINED ENV{ARC_ARDUINO_PATH} AND
   EXISTS "$ENV{ARC_ARDUINO_PATH}/CMakeLists.txt" AND
   EXISTS "$ENV{ARC_ARDUINO_PATH}/idf_component.yml")
    set(_arc_bench_arduino_dir "$ENV{ARC_ARDUINO_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../arduino-esp32/CMakeLists.txt" AND
       EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../arduino-esp32/idf_component.yml")
    set(_arc_bench_arduino_dir "${CMAKE_CURRENT_LIST_DIR}/../../arduino-esp32")
endif()

if(NOT _arc_bench_arduino_dir STREQUAL "")
    list(APPEND ARC_COMPONENT_DIRS "${_arc_bench_arduino_dir}")
    list(APPEND ARC_SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults.arduino")
    set(ARC_BENCH_HAS_ARDUINO ON)
    message(STATUS "arc_bench: enabling Arduino benchmark component from ${_arc_bench_arduino_dir}")
else()
    message(STATUS "arc_bench: Arduino benchmark component not found; firmware will compare Arc with raw ESP-IDF only")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/arc-idf.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(arc_bench)
