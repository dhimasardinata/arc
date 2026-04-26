#pragma once

#include <cstdint>

#include "esp_cpu.h"
#include "esp_err.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "hal/trace_ll.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "sdkconfig.h"
#include "xt_trax.h"
#include "xtensa/config/core-isa.h"

namespace arc {

struct Trax {
    static_assert(XCHAL_HAVE_TRAX != 0, "Xtensa TRAX is not supported on this target");

    enum class Bank : std::uint8_t {
        pro,
        app,
        pro_app,
        app_pro,
    };

    enum class Unit : std::uint8_t {
        word,
        inst,
    };

    [[nodiscard]] static esp_err_t enable() noexcept
    {
        return enable(core());
    }

    [[nodiscard]] static esp_err_t enable(const Bank bank) noexcept
    {
#if !defined(CONFIG_ESP32S3_TRAX)
        static_cast<void>(bank);
        return ESP_ERR_NO_MEM;
#else
        switch (bank) {
            case Bank::pro:
                trace_ll_set_mem_block(0, TRACEMEM_MUX_BLK0_NUM);
                return ESP_OK;
            case Bank::app:
                trace_ll_set_mem_block(1, TRACEMEM_MUX_BLK0_NUM);
                return ESP_OK;
            case Bank::pro_app:
#if defined(CONFIG_ESP32S3_TRAX_TWOBANKS)
                trace_ll_set_mem_block(0, TRACEMEM_MUX_BLK0_NUM);
                trace_ll_set_mem_block(1, TRACEMEM_MUX_BLK1_NUM);
                return ESP_OK;
#else
                return ESP_ERR_INVALID_ARG;
#endif
            case Bank::app_pro:
#if defined(CONFIG_ESP32S3_TRAX_TWOBANKS)
                trace_ll_set_mem_block(1, TRACEMEM_MUX_BLK0_NUM);
                trace_ll_set_mem_block(0, TRACEMEM_MUX_BLK1_NUM);
                return ESP_OK;
#else
                return ESP_ERR_INVALID_ARG;
#endif
        }

        return ESP_ERR_INVALID_ARG;
#endif
    }

    [[nodiscard]] static esp_err_t both(const bool swap = false) noexcept
    {
        return enable(swap ? Bank::app_pro : Bank::pro_app);
    }

    [[nodiscard]] static esp_err_t start(const Unit unit = Unit::word) noexcept
    {
#if !defined(CONFIG_ESP32S3_TRAX)
        static_cast<void>(unit);
        return ESP_ERR_NO_MEM;
#else
        if (active()) {
            xt_trax_trigger_traceend_after_delay(0);
        }

        if (unit == Unit::inst) {
            xt_trax_start_trace_instructions();
        } else {
            xt_trax_start_trace_words();
        }
        return ESP_OK;
#endif
    }

    [[nodiscard]] static esp_err_t words() noexcept
    {
        return start(Unit::word);
    }

    [[nodiscard]] static esp_err_t instr() noexcept
    {
        return start(Unit::inst);
    }

    [[nodiscard]] static bool active() noexcept
    {
        return xt_trax_trace_is_active();
    }

    [[nodiscard]] static esp_err_t stop(const int delay = 0) noexcept
    {
#if !defined(CONFIG_ESP32S3_TRAX)
        static_cast<void>(delay);
        return ESP_ERR_NO_MEM;
#else
        xt_trax_trigger_traceend_after_delay(delay);
        return ESP_OK;
#endif
    }

    [[nodiscard]] static Bank core(const int id = esp_cpu_get_core_id()) noexcept
    {
        return id == 0 ? Bank::pro : Bank::app;
    }
};

}  // namespace arc
