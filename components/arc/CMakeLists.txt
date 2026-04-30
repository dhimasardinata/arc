idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_hw_support esp_mm esp_rom freertos heap log soc xtensa
)

set(ARC_STACK_USAGE_LIMIT "2048" CACHE STRING "Maximum static stack usage allowed for one C++ function")
set(ARC_STACK_FRAME_LIMIT "1024" CACHE STRING "Maximum C++ stack frame size allowed before the compiler errors")

target_compile_options(${COMPONENT_LIB} INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
    $<$<COMPILE_LANGUAGE:CXX>:-Werror=stack-usage=${ARC_STACK_USAGE_LIMIT}>
    $<$<COMPILE_LANGUAGE:CXX>:-Werror=frame-larger-than=${ARC_STACK_FRAME_LIMIT}>
)
