include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/arc-deps.cmake)

arc_requires(main_requires core rng time sha aes copy temp store ota twai)

if(ARC_BENCH_HAS_ARDUINO)
    list(APPEND main_requires arduino-esp32)
endif()

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)

if(ARC_BENCH_HAS_ARDUINO)
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
