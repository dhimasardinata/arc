include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/arc-deps.cmake)

arc_requires(main_requires core rmt)

idf_component_register(
    SRCS "app_main.cpp"
    REQUIRES ${main_requires}
)

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
