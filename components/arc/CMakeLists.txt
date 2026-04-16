idf_component_register(
    INCLUDE_DIRS "include"
    REQUIRES app_update esp_partition freertos heap esp_driver_gpio esp_event esp_hw_support esp_netif esp_wifi lwip nvs_flash soc spi_flash
)

target_compile_options(${COMPONENT_LIB} INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26>
    $<$<COMPILE_LANGUAGE:CXX>:-mtext-section-literals>
)
