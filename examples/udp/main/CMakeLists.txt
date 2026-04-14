set(main_requires
    arc
    esp_event
    esp_netif
    esp_wifi
    lwip
    nvs_flash
)

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
