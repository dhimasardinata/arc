idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES esp_common esp_hw_support esp_mm esp_rom freertos heap log soc xtensa
)

target_compile_options(${COMPONENT_LIB} INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
)
