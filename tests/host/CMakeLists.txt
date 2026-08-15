cmake_minimum_required(VERSION 3.20)
project(arc-host-tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(arc_host_flags INTERFACE)
target_compile_options(arc_host_flags INTERFACE -Wall -Wextra -Werror)
target_include_directories(
    arc_host_flags
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/stubs
        ${CMAKE_CURRENT_LIST_DIR}/../../components/arc/include
)
target_link_libraries(arc_host_flags INTERFACE pthread)

add_executable(
    arc-host-logic
    logic.cpp
    plane_compile.cpp
    safety_patterns_compile.cpp
    esp32s31_compile.cpp
    fuzz_codecs.cpp
)
target_link_libraries(arc-host-logic PRIVATE arc_host_flags)
target_compile_options(arc-host-logic PRIVATE -O2 -g)

set(ARC_S31_EXAMPLES amp audio cam console control io lcd ml ptp radio sd security usb)
set(ARC_S31_EXAMPLE_TARGETS)
foreach(example IN LISTS ARC_S31_EXAMPLES)
    add_library(
        arc-host-s31-example-${example}
        OBJECT
        ${CMAKE_CURRENT_LIST_DIR}/../../examples/esp32s31/${example}/main/app_main.cpp
    )
    target_link_libraries(arc-host-s31-example-${example} PRIVATE arc_host_flags)
    target_compile_definitions(arc-host-s31-example-${example} PRIVATE ARC_TARGET_ESP32S31=1)
    target_compile_options(arc-host-s31-example-${example} PRIVATE -O2 -g)
    list(APPEND ARC_S31_EXAMPLE_TARGETS arc-host-s31-example-${example})
endforeach()
add_custom_target(arc-host-s31-examples)
add_dependencies(arc-host-s31-examples ${ARC_S31_EXAMPLE_TARGETS})
add_dependencies(arc-host-logic arc-host-s31-examples)

add_executable(arc-host-bench bench.cpp)
target_link_libraries(arc-host-bench PRIVATE arc_host_flags)
target_compile_options(arc-host-bench PRIVATE -O3)
target_compile_definitions(arc-host-bench PRIVATE NDEBUG)

option(ARC_HOST_ENABLE_FUZZ "Build libFuzzer codec harnesses" OFF)
if(ARC_HOST_ENABLE_FUZZ)
    add_executable(arc-host-fuzz-codecs fuzz_codecs.cpp)
    target_link_libraries(arc-host-fuzz-codecs PRIVATE arc_host_flags)
    target_compile_options(arc-host-fuzz-codecs PRIVATE -O1 -g -fsanitize=fuzzer,address,undefined)
    target_link_options(arc-host-fuzz-codecs PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
