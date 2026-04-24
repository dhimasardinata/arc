#pragma once

#include <cstdint>

#include "soc/soc_caps.h"

namespace arc {

struct Soc {
    static constexpr std::uint32_t cores = SOC_CPU_CORES_NUM;
    static constexpr bool multicore = SOC_HP_CPU_HAS_MULTIPLE_CORES;
    static constexpr bool fpu = SOC_CPU_HAS_FPU;
    static constexpr bool simd = SOC_SIMD_INSTRUCTION_SUPPORTED;

    static constexpr bool wifi = SOC_WIFI_SUPPORTED;
    static constexpr bool bluetooth = SOC_BT_SUPPORTED;
    static constexpr bool ble = SOC_BLE_SUPPORTED;
    static constexpr bool ble5 = SOC_BLE_50_SUPPORTED;

    static constexpr bool dedicated_gpio = SOC_DEDICATED_GPIO_SUPPORTED;
    static constexpr bool async_memcpy = SOC_ASYNC_MEMCPY_SUPPORTED;
    static constexpr bool gdma = SOC_GDMA_SUPPORTED;
    static constexpr bool ahb_gdma = SOC_AHB_GDMA_SUPPORTED;
    static constexpr bool psram_dma = SOC_PSRAM_DMA_CAPABLE;

    static constexpr bool adc = SOC_ADC_SUPPORTED;
    static constexpr bool adc_dma = SOC_ADC_DMA_SUPPORTED;
    static constexpr bool i2c = SOC_I2C_SUPPORTED;
    static constexpr bool i2s = SOC_I2S_SUPPORTED;
    static constexpr bool spi = SOC_GPSPI_SUPPORTED;
    static constexpr bool ledc = SOC_LEDC_SUPPORTED;
    static constexpr bool lcd_i80 = SOC_LCD_I80_SUPPORTED;
    static constexpr bool lcd_rgb = SOC_LCD_RGB_SUPPORTED;
    static constexpr bool lcdcam_dvp = SOC_LCDCAM_CAM_SUPPORTED;
    static constexpr bool mcpwm = SOC_MCPWM_SUPPORTED;
    static constexpr bool pcnt = SOC_PCNT_SUPPORTED;
    static constexpr bool rmt = SOC_RMT_SUPPORTED;
    static constexpr bool rmt_dma = SOC_RMT_SUPPORT_DMA;
    static constexpr bool sdm = SOC_SDM_SUPPORTED;
    static constexpr bool timer = SOC_GPTIMER_SUPPORTED;
    static constexpr bool twai = SOC_TWAI_SUPPORTED;
    static constexpr bool temp = SOC_TEMP_SENSOR_SUPPORTED;
    static constexpr bool touch = SOC_TOUCH_SENSOR_SUPPORTED;
    static constexpr bool uart = SOC_UART_SUPPORTED;
    static constexpr bool usb_otg = SOC_USB_OTG_SUPPORTED;
    static constexpr bool usb_serial_jtag = SOC_USB_SERIAL_JTAG_SUPPORTED;
    static constexpr bool sdmmc = SOC_SDMMC_HOST_SUPPORTED;
    static constexpr bool rng = SOC_RNG_SUPPORTED;
    static constexpr bool aes = SOC_AES_SUPPORTED;
    static constexpr bool sha = SOC_SHA_SUPPORTED;
    static constexpr bool hmac = SOC_HMAC_SUPPORTED;
    static constexpr bool efuse = SOC_EFUSE_SUPPORTED;
    static constexpr bool systimer = SOC_SYSTIMER_SUPPORTED;
    static constexpr bool wdt = SOC_WDT_SUPPORTED;
    static constexpr bool xt_wdt = SOC_XT_WDT_SUPPORTED;

    static constexpr bool light_sleep = SOC_LIGHT_SLEEP_SUPPORTED;
    static constexpr bool deep_sleep = SOC_DEEP_SLEEP_SUPPORTED;
    static constexpr bool ulp = SOC_ULP_SUPPORTED;
    static constexpr bool ulp_riscv = SOC_RISCV_COPROC_SUPPORTED;
    static constexpr bool rtc_fast = SOC_RTC_FAST_MEM_SUPPORTED;
    static constexpr bool rtc_slow = SOC_RTC_SLOW_MEM_SUPPORTED;

    static constexpr std::uint32_t cpu_interrupts = SOC_CPU_INTR_NUM;
    static constexpr std::uint32_t gpio_pins = SOC_GPIO_PIN_COUNT;
    static constexpr std::uint32_t gpio_input_max = SOC_GPIO_IN_RANGE_MAX;
    static constexpr std::uint32_t gpio_output_max = SOC_GPIO_OUT_RANGE_MAX;
    static constexpr std::uint32_t adc_units = SOC_ADC_PERIPH_NUM;
    static constexpr std::uint32_t adc_channels = SOC_ADC_MAX_CHANNEL_NUM;
    static constexpr std::uint32_t adc_pattern = SOC_ADC_PATT_LEN_MAX;
    static constexpr std::uint32_t adc_bits = SOC_ADC_DIGI_MAX_BITWIDTH;
    static constexpr std::uint32_t i2c_ports = SOC_I2C_NUM;
    static constexpr std::uint32_t ledc_timers = SOC_LEDC_TIMER_NUM;
    static constexpr std::uint32_t ledc_channels = SOC_LEDC_CHANNEL_NUM;
    static constexpr std::uint32_t rmt_words = SOC_RMT_MEM_WORDS_PER_CHANNEL;
    static constexpr std::uint32_t spi_peripherals = SOC_SPI_PERIPH_NUM;
    static constexpr std::uint32_t spi_max_cs = SOC_SPI_MAX_CS_NUM;
    static constexpr std::uint32_t spi_fifo_bytes = SOC_SPI_MAXIMUM_BUFFER_SIZE;
    static constexpr std::uint32_t systimer_counters = SOC_SYSTIMER_COUNTER_NUM;
    static constexpr std::uint32_t systimer_alarms = SOC_SYSTIMER_ALARM_NUM;
    static constexpr std::uint32_t twai_controllers = SOC_TWAI_CONTROLLER_NUM;
    static constexpr std::uint32_t uart_ports = SOC_UART_NUM;
    static constexpr std::uint32_t uart_max_bitrate = SOC_UART_BITRATE_MAX;
    static constexpr std::uint32_t sdmmc_slots = SOC_SDMMC_NUM_SLOTS;
    static constexpr std::uint32_t sdmmc_width = SOC_SDMMC_DATA_WIDTH_MAX;
    static constexpr std::uint32_t touch_max_channel = SOC_TOUCH_MAX_CHAN_ID;
};

static_assert(Soc::cores == 2U, "Arc targets the dual-core ESP32-S3");
static_assert(Soc::dedicated_gpio, "Arc requires ESP32-S3 dedicated GPIO");
static_assert(Soc::async_memcpy && Soc::ahb_gdma, "Arc requires ESP32-S3 async AHB-GDMA");
static_assert(Soc::simd, "Arc expects ESP32-S3 SIMD instructions");

}  // namespace arc
