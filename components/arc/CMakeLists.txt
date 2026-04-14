idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES freertos heap esp_driver_gpio esp_hw_support soc
)

target_compile_options(${COMPONENT_LIB} INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
)
