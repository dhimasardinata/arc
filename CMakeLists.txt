cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH is not set. Source the ESP-IDF export script before configuring this project.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(arc)
