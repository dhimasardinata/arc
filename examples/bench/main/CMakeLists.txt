include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/arc-deps.cmake)

arc_requires(main_requires core rng time sha aes copy temp store ota twai)

set(_bench_has_arduino OFF)
if((DEFINED ENV{ARC_ARDUINO_PATH} AND
    EXISTS "$ENV{ARC_ARDUINO_PATH}/CMakeLists.txt" AND
    EXISTS "$ENV{ARC_ARDUINO_PATH}/idf_component.yml") OR
   (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../arduino-esp32/CMakeLists.txt" AND
    EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../../arduino-esp32/idf_component.yml"))
    set(_bench_has_arduino ON)
endif()

if(_bench_has_arduino)
    list(APPEND main_requires arduino-esp32)
endif()

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)

if(_bench_has_arduino)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ARC_BENCH_HAS_ARDUINO=1)
endif()

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wconversion
    -Wsign-conversion
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)
