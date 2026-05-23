#pragma once

#include <cstdint>

#include "arc/soc/target.hpp"
#include "soc/soc_caps.h"

namespace arc {

struct Soc {
    using Chip = soc::Target;

    static constexpr const char* target = Chip::name;
    static constexpr const char* arch = Chip::arch;
    static constexpr bool experimental = Chip::experimental;

    static constexpr std::uint32_t cores = SOC_CPU_CORES_NUM;
    static constexpr bool multicore = SOC_HP_CPU_HAS_MULTIPLE_CORES;
    static constexpr bool fpu = SOC_CPU_HAS_FPU;
    static constexpr bool simd = SOC_SIMD_INSTRUCTION_SUPPORTED;

#if defined(SOC_WIFI_SUPPORTED)
    static constexpr bool wifi = SOC_WIFI_SUPPORTED;
#else
    static constexpr bool wifi = false;
#endif
#if defined(SOC_BT_SUPPORTED)
    static constexpr bool bluetooth = SOC_BT_SUPPORTED;
#else
    static constexpr bool bluetooth = false;
#endif
#if defined(SOC_BLE_SUPPORTED)
    static constexpr bool ble = SOC_BLE_SUPPORTED;
#else
    static constexpr bool ble = false;
#endif
#if defined(SOC_BLE_50_SUPPORTED)
    static constexpr bool ble5 = SOC_BLE_50_SUPPORTED;
#else
    static constexpr bool ble5 = false;
#endif
#if defined(SOC_BLE_MESH_SUPPORTED)
    static constexpr bool ble_mesh = SOC_BLE_MESH_SUPPORTED;
#else
    static constexpr bool ble_mesh = false;
#endif
#if defined(SOC_BLE_DEVICE_PRIVACY_SUPPORTED)
    static constexpr bool ble_privacy = SOC_BLE_DEVICE_PRIVACY_SUPPORTED;
#else
    static constexpr bool ble_privacy = false;
#endif

#if defined(SOC_ETM_SUPPORTED)
    static constexpr bool etm = SOC_ETM_SUPPORTED;
#else
    static constexpr bool etm = false;
#endif

    static constexpr bool fast_gpio = SOC_DEDICATED_GPIO_SUPPORTED;
    static constexpr bool async_copy = SOC_ASYNC_MEMCPY_SUPPORTED;
    static constexpr bool gdma = SOC_GDMA_SUPPORTED;
    static constexpr bool ahb_dma = SOC_AHB_GDMA_SUPPORTED;
    static constexpr bool psram_dma = SOC_PSRAM_DMA_CAPABLE;
    static constexpr bool dma2d = Chip::dma2d;
    static constexpr bool ppa = Chip::ppa;
    static constexpr bool jpeg = Chip::jpeg;
    static constexpr bool h264 = Chip::h264;

    static constexpr bool adc = SOC_ADC_SUPPORTED;
    static constexpr bool adc_dma = SOC_ADC_DMA_SUPPORTED;
    static constexpr bool i2c = SOC_I2C_SUPPORTED;
    static constexpr bool i2s = SOC_I2S_SUPPORTED;
    static constexpr bool spi = SOC_GPSPI_SUPPORTED;
    static constexpr bool ledc = SOC_LEDC_SUPPORTED;
    static constexpr bool lcd_i80 = SOC_LCD_I80_SUPPORTED;
    static constexpr bool lcd_rgb = SOC_LCD_RGB_SUPPORTED;
    static constexpr bool dvp = SOC_LCDCAM_CAM_SUPPORTED;
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
    static constexpr bool usb_jtag = SOC_USB_SERIAL_JTAG_SUPPORTED;
    static constexpr bool sdmmc = SOC_SDMMC_HOST_SUPPORTED;
    static constexpr bool rng = SOC_RNG_SUPPORTED;
    static constexpr bool aes = SOC_AES_SUPPORTED;
    static constexpr bool aes_dma = SOC_AES_SUPPORT_DMA;
    static constexpr bool aes_128 = SOC_AES_SUPPORT_AES_128;
    static constexpr bool aes_256 = SOC_AES_SUPPORT_AES_256;
    static constexpr bool sha = SOC_SHA_SUPPORTED;
    static constexpr bool sha_dma = SOC_SHA_SUPPORT_DMA;
    static constexpr bool sha1 = SOC_SHA_SUPPORT_SHA1;
    static constexpr bool sha224 = SOC_SHA_SUPPORT_SHA224;
    static constexpr bool sha256 = SOC_SHA_SUPPORT_SHA256;
    static constexpr bool sha384 = SOC_SHA_SUPPORT_SHA384;
    static constexpr bool sha512 = SOC_SHA_SUPPORT_SHA512;
    static constexpr bool sha512_224 = SOC_SHA_SUPPORT_SHA512_224;
    static constexpr bool sha512_256 = SOC_SHA_SUPPORT_SHA512_256;
    static constexpr bool sha512_t = SOC_SHA_SUPPORT_SHA512_T;
    static constexpr bool mpi = SOC_MPI_SUPPORTED;
    static constexpr bool hmac = SOC_HMAC_SUPPORTED;
    static constexpr bool sign = SOC_DIG_SIGN_SUPPORTED;
#if defined(SOC_ECDSA_SUPPORTED)
    static constexpr bool ecdsa = SOC_ECDSA_SUPPORTED;
#else
    static constexpr bool ecdsa = false;
#endif
    static constexpr bool efuse = SOC_EFUSE_SUPPORTED;
    static constexpr bool xts = SOC_FLASH_ENCRYPTION_XTS_AES;
    static constexpr bool xts128 = SOC_FLASH_ENCRYPTION_XTS_AES_128;
    static constexpr bool xts256 = SOC_FLASH_ENCRYPTION_XTS_AES_256;
    static constexpr bool systimer = SOC_SYSTIMER_SUPPORTED;
    static constexpr bool wdt = SOC_WDT_SUPPORTED;
#if defined(SOC_XT_WDT_SUPPORTED)
    static constexpr bool xt_wdt = SOC_XT_WDT_SUPPORTED;
#else
    static constexpr bool xt_wdt = false;
#endif

    static constexpr bool light_sleep = SOC_LIGHT_SLEEP_SUPPORTED;
    static constexpr bool deep_sleep = SOC_DEEP_SLEEP_SUPPORTED;
    static constexpr bool ulp = SOC_ULP_SUPPORTED;
#if defined(SOC_ULP_FSM_SUPPORTED)
    static constexpr bool ulp_fsm = SOC_ULP_FSM_SUPPORTED;
#else
    static constexpr bool ulp_fsm = false;
#endif
#if defined(SOC_RISCV_COPROC_SUPPORTED)
    static constexpr bool ulp_riscv = SOC_RISCV_COPROC_SUPPORTED;
#else
    static constexpr bool ulp_riscv = false;
#endif
    static constexpr bool rtc_fast = SOC_RTC_FAST_MEM_SUPPORTED;
#if defined(SOC_RTC_SLOW_MEM_SUPPORTED)
    static constexpr bool rtc_slow = SOC_RTC_SLOW_MEM_SUPPORTED;
#else
    static constexpr bool rtc_slow = false;
#endif
    static constexpr bool rtc_gpio = SOC_RTCIO_PIN_COUNT > 0;
    static constexpr bool rtc_io = SOC_RTCIO_INPUT_OUTPUT_SUPPORTED;
    static constexpr bool rtc_hold = SOC_RTCIO_HOLD_SUPPORTED;
    static constexpr bool rtc_wake = SOC_RTCIO_WAKE_SUPPORTED;

    static constexpr std::uint32_t cpu_interrupts = SOC_CPU_INTR_NUM;
    static constexpr std::uint32_t gpio_pins = SOC_GPIO_PIN_COUNT;
    static constexpr std::uint32_t rtc_pins = SOC_RTCIO_PIN_COUNT;
    static constexpr std::uint32_t gpio_in = SOC_GPIO_IN_RANGE_MAX;
    static constexpr std::uint32_t gpio_out = SOC_GPIO_OUT_RANGE_MAX;
    static constexpr std::uint32_t adc_units = SOC_ADC_PERIPH_NUM;
    static constexpr std::uint32_t adc_channels = SOC_ADC_MAX_CHANNEL_NUM;
    static constexpr std::uint32_t adc_pattern = SOC_ADC_PATT_LEN_MAX;
    static constexpr std::uint32_t adc_bits = SOC_ADC_DIGI_MAX_BITWIDTH;
    static constexpr std::uint32_t i2c_ports = SOC_I2C_NUM;
    static constexpr std::uint32_t ledc_timers = SOC_LEDC_TIMER_NUM;
    static constexpr std::uint32_t ledc_channels = SOC_LEDC_CHANNEL_NUM;
    static constexpr std::uint32_t rmt_words = SOC_RMT_MEM_WORDS_PER_CHANNEL;
    static constexpr std::uint32_t spi_ports = SOC_SPI_PERIPH_NUM;
    static constexpr std::uint32_t spi_cs = SOC_SPI_MAX_CS_NUM;
    static constexpr std::uint32_t spi_fifo = SOC_SPI_MAXIMUM_BUFFER_SIZE;
    static constexpr std::uint32_t systimer_counters = SOC_SYSTIMER_COUNTER_NUM;
    static constexpr std::uint32_t systimer_alarms = SOC_SYSTIMER_ALARM_NUM;
    static constexpr std::uint32_t rsa_bits = SOC_RSA_MAX_BIT_LEN;
    static constexpr std::uint32_t ds_bits = SOC_DS_SIGNATURE_MAX_BIT_LEN;
    static constexpr std::uint32_t ds_iv = SOC_DS_KEY_PARAM_MD_IV_LENGTH;
    static constexpr std::uint32_t ds_wait = SOC_DS_KEY_CHECK_MAX_WAIT_US;
    static constexpr std::uint32_t xts_block = SOC_FLASH_ENCRYPTED_XTS_AES_BLOCK_MAX;
    static constexpr std::uint32_t mpi_blocks = SOC_MPI_MEM_BLOCKS_NUM;
    static constexpr std::uint32_t mpi_ops = SOC_MPI_OPERATIONS_NUM;
    static constexpr std::uint32_t twai_controllers = SOC_TWAI_CONTROLLER_NUM;
    static constexpr std::uint32_t uart_ports = SOC_UART_NUM;
    static constexpr std::uint32_t uart_bitrate = SOC_UART_BITRATE_MAX;
    static constexpr std::uint32_t sdmmc_slots = SOC_SDMMC_NUM_SLOTS;
    static constexpr std::uint32_t sdmmc_width = SOC_SDMMC_DATA_WIDTH_MAX;
    static constexpr std::uint32_t touch_max = SOC_TOUCH_MAX_CHAN_ID;
};

static_assert(!soc::s3 || Soc::cores == 2U, "Arc expects dual-core ESP32-S3");
static_assert(!soc::s3 || Soc::fast_gpio, "Arc requires ESP32-S3 dedicated GPIO");
static_assert(!soc::s3 || (Soc::async_copy && Soc::ahb_dma), "Arc requires ESP32-S3 async AHB-GDMA");
static_assert(!soc::s3 || Soc::simd, "Arc expects ESP32-S3 SIMD instructions");

}  // namespace arc
