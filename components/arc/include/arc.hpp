#pragma once

#include "arc/bus.hpp"
#include "arc/cache.hpp"
#include "arc/caps.hpp"
#include "arc/cfg.hpp"
#include "arc/clock.hpp"
#include "arc/dsp.hpp"
#include "arc/fence.hpp"
#include "arc/mask.hpp"
#include "arc/place.hpp"
#include "arc/plane.hpp"
#include "arc/probe.hpp"
#include "arc/reg.hpp"
#include "arc/ring.hpp"
#include "arc/sdk.hpp"
#include "arc/seq.hpp"
#include "arc/sketch.hpp"
#include "arc/task.hpp"
#include "arc/tight.hpp"
#include "arc/wave.hpp"

#if __has_include("driver/gpio.h")
#include "arc/gpio.hpp"
#endif

#if __has_include("esp_private/gpio.h") && __has_include("esp_rom_gpio.h") && __has_include("hal/dedic_gpio_cpu_ll.h")
#include "arc/din.hpp"
#include "arc/dio.hpp"
#endif

#if __has_include("esp_adc/adc_continuous.h")
#include "arc/adc.hpp"
#include "arc/scope.hpp"
#endif

#if __has_include("driver/mcpwm_prelude.h")
#include "arc/bridge.hpp"
#include "arc/capture.hpp"
#include "arc/pulse.hpp"
#endif

#if __has_include("driver/pulse_cnt.h")
#include "arc/count.hpp"
#endif

#if __has_include("esp_async_memcpy.h")
#include "arc/copy.hpp"
#endif

#if __has_include("esp_cam_ctlr.h") && __has_include("esp_cam_ctlr_dvp.h")
#include "arc/dvp.hpp"
#endif

#if __has_include("driver/i2s_std.h")
#include "arc/i2s.hpp"
#endif

#if __has_include("esp_lcd_io_i80.h") && __has_include("esp_lcd_panel_io.h")
#include "arc/i80.hpp"
#endif

#if __has_include("esp_ota_ops.h") && __has_include("esp_partition.h")
#include "arc/ota.hpp"
#endif

#if __has_include("driver/ledc.h")
#include "arc/pwm.hpp"
#endif

#if __has_include("driver/rmt_tx.h")
#include "arc/burst.hpp"
#endif

#if __has_include("driver/rmt_rx.h")
#include "arc/trace.hpp"
#endif

#if __has_include("driver/sdm.h")
#include "arc/sigma.hpp"
#endif

#if __has_include("esp_sleep.h")
#include "arc/sleep.hpp"
#endif

#if __has_include("esp_flash.h") && __has_include("esp_image_format.h") && __has_include("esp_ota_ops.h") && __has_include("esp_partition.h")
#include "arc/space.hpp"
#endif

#if __has_include("driver/spi_master.h")
#include "arc/spi.hpp"
#endif

#if __has_include("nvs.h") && __has_include("nvs_flash.h")
#include "arc/store.hpp"
#endif

#if __has_include("driver/temperature_sensor.h")
#include "arc/temp.hpp"
#endif

#if __has_include("driver/gptimer.h")
#include "arc/timer.hpp"
#endif

#if __has_include("esp_twai.h") && __has_include("esp_twai_onchip.h")
#include "arc/can.hpp"
#endif

#if __has_include("esp_event.h") && __has_include("esp_netif.h") && __has_include("esp_now.h") && __has_include("esp_wifi.h") && __has_include("nvs_flash.h")
#include "arc/espnow.hpp"
#endif

#if __has_include("lwip/netdb.h") && __has_include("lwip/sockets.h") && __has_include("esp_event.h") && __has_include("esp_netif.h") && __has_include("esp_wifi.h") && __has_include("nvs_flash.h")
#include "arc/udp.hpp"
#endif
