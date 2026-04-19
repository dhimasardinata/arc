cmake_minimum_required(VERSION 3.16)

set(ARC_SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults"
    "${CMAKE_CURRENT_LIST_DIR}/main/sdkconfig.defaults")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/arc-idf.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(arc)
