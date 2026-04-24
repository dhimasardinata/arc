#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "esp_err.h"
#include "soc/soc_caps.h"
#include "ulp_riscv.h"

namespace arc {

struct Ulp {
    static_assert(SOC_RISCV_COPROC_SUPPORTED, "ULP RISC-V is not supported on this target");

    struct Word {
        alignas(4) mutable std::uint32_t value{};

        [[nodiscard]] std::uint32_t read() const noexcept
        {
            return __atomic_load_n(&value, __ATOMIC_ACQUIRE);
        }

        void write(const std::uint32_t next) noexcept
        {
            __atomic_store_n(&value, next, __ATOMIC_RELEASE);
        }

        [[nodiscard]] std::uint32_t add(const std::uint32_t delta = 1U) noexcept
        {
            return __atomic_add_fetch(&value, delta, __ATOMIC_ACQ_REL);
        }
    };

    [[nodiscard]] static esp_err_t load(
        const void* const data,
        const std::size_t bytes) noexcept
    {
        if (data == nullptr || bytes == 0U) {
            return ESP_ERR_INVALID_ARG;
        }

        return ulp_riscv_load_binary(static_cast<const std::uint8_t*>(data), bytes);
    }

    template <typename T, std::size_t Extent>
    [[nodiscard]] static esp_err_t load(const std::span<T, Extent> data) noexcept
    {
        return load(data.data(), data.size_bytes());
    }

    [[nodiscard]] static esp_err_t run(
        const ulp_riscv_wakeup_source_t wake = ULP_RISCV_WAKEUP_SOURCE_TIMER) noexcept
    {
        ulp_riscv_cfg_t config{};
        config.wakeup_source = wake;
        return ulp_riscv_config_and_run(&config);
    }

    [[nodiscard]] static esp_err_t start() noexcept
    {
        return ulp_riscv_run();
    }

    static void stop() noexcept
    {
        ulp_riscv_timer_stop();
    }

    static void resume() noexcept
    {
        ulp_riscv_timer_resume();
    }

    static void halt() noexcept
    {
        ulp_riscv_halt();
    }

    static void reset() noexcept
    {
        ulp_riscv_reset();
    }

    [[nodiscard]] static esp_err_t isr(
        intr_handler_t handler,
        void* const arg = nullptr,
        const std::uint32_t mask = ULP_RISCV_SW_INT | ULP_RISCV_TRAP_INT) noexcept
    {
        return ulp_riscv_isr_register(handler, arg, mask);
    }

    [[nodiscard]] static esp_err_t off(
        intr_handler_t handler,
        void* const arg = nullptr,
        const std::uint32_t mask = ULP_RISCV_SW_INT | ULP_RISCV_TRAP_INT) noexcept
    {
        return ulp_riscv_isr_deregister(handler, arg, mask);
    }
};

}  // namespace arc
