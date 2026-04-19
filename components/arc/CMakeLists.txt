idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES app_update bootloader_support esp_partition freertos heap esp_driver_gpio esp_driver_gptimer esp_driver_ledc esp_driver_pcnt esp_driver_rmt esp_hw_support nvs_flash soc spi_flash
)

target_compile_options(${COMPONENT_LIB} INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
)
