if(NOT DEFINED ARC_TARGET OR ARC_TARGET STREQUAL "")
    set(ARC_TARGET "esp32s3")
endif()

set(arc_component_requires esp_common esp_hw_support esp_mm esp_rom freertos heap log soc)
set(arc_consumer_definitions)

if(ARC_TARGET STREQUAL "esp32s31")
    list(APPEND arc_consumer_definitions ARC_TARGET_ESP32S31=1 ARC_TARGET_NAME="esp32s31")
else()
    list(APPEND arc_component_requires xtensa)
    list(APPEND arc_consumer_definitions ARC_TARGET_ESP32S3=1 ARC_TARGET_NAME="esp32s3")
endif()

if(ARC_EXPERIMENTAL_ESP32S31)
    list(APPEND arc_consumer_definitions ARC_EXPERIMENTAL_ESP32S31=1)
endif()

idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES ${arc_component_requires}
)

set(ARC_STACK_USAGE_LIMIT "2048" CACHE STRING "Maximum static stack usage allowed for one C++ function")
set(ARC_STACK_FRAME_LIMIT "1024" CACHE STRING "Maximum C++ stack frame size allowed before the compiler errors")
option(ARC_ENFORCE_CONSUMER_CXX26 "Force Arc consumers to build C++ units as gnu++26" ON)
option(ARC_ENFORCE_CONSUMER_TEXT_LITERALS "Force Arc consumer C++ units to use Xtensa text-section literals" ON)
option(ARC_ENFORCE_STACK_LIMITS "Force Arc consumer C++ stack/frame warning errors" ON)

set(arc_consumer_compile_options)

if(ARC_ENFORCE_CONSUMER_CXX26)
    list(APPEND arc_consumer_compile_options
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    )
endif()

if(ARC_ENFORCE_CONSUMER_TEXT_LITERALS AND NOT ARC_TARGET STREQUAL "esp32s31")
    list(APPEND arc_consumer_compile_options
        $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
    )
endif()

if(ARC_ENFORCE_STACK_LIMITS)
    list(APPEND arc_consumer_compile_options
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=stack-usage=${ARC_STACK_USAGE_LIMIT}>
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=frame-larger-than=${ARC_STACK_FRAME_LIMIT}>
    )
endif()

target_compile_definitions(${COMPONENT_LIB} INTERFACE ${arc_consumer_definitions})
target_compile_options(${COMPONENT_LIB} INTERFACE ${arc_consumer_compile_options})
