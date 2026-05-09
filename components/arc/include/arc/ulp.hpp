#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"
#include "esp_attr.h"
#include "soc/soc_caps.h"
#include "ulp.h"
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

    template <typename T>
    struct alignas(8) Shared {
        static_assert(std::is_trivially_copyable_v<T>, "ULP shared payload must be trivially copyable");
        static_assert(std::is_trivially_destructible_v<T>, "ULP shared payload must be trivially destructible");
        static_assert((sizeof(T) % 4U) == 0U, "ULP shared payload size must be word aligned");

        using value_type = T;

        Word seq{};
        alignas(8) T value{};

        void write(const T& next) noexcept
        {
            static_cast<void>(seq.add());
            value = next;
            static_cast<void>(seq.add());
        }

        [[nodiscard]] bool read(T& out) const noexcept
        {
            const auto head = seq.read();
            if ((head & 1U) != 0U) {
                return false;
            }
            out = value;
            __atomic_thread_fence(__ATOMIC_ACQUIRE);
            return head == seq.read();
        }
    };

    struct Fsm {
        static_assert(SOC_ULP_FSM_SUPPORTED, "ULP FSM is not supported on this target");

        using Insn = ulp_insn_t;

        [[nodiscard]] static esp_err_t period(
            const std::size_t index,
            const std::uint32_t us) noexcept
        {
            return ulp_set_wakeup_period(index, us);
        }

        [[nodiscard]] static esp_err_t load(
            const std::uint32_t addr,
            const void* const data,
            const std::size_t bytes) noexcept
        {
            if (data == nullptr || bytes == 0U) {
                return ESP_ERR_INVALID_ARG;
            }

            return ulp_load_binary(addr, static_cast<const std::uint8_t*>(data), bytes);
        }

        [[nodiscard]] static esp_err_t load(
            const void* const data,
            const std::size_t bytes) noexcept
        {
            return load(0U, data, bytes);
        }

        template <typename T, std::size_t Extent>
        [[nodiscard]] static esp_err_t load(
            const std::uint32_t addr,
            const std::span<T, Extent> data) noexcept
        {
            return load(addr, data.data(), data.size_bytes());
        }

        template <typename T, std::size_t Extent>
        [[nodiscard]] static esp_err_t load(const std::span<T, Extent> data) noexcept
        {
            return load(0U, data);
        }

        [[nodiscard]] static esp_err_t macro(
            const std::uint32_t addr,
            const Insn* const program,
            std::size_t& words) noexcept
        {
            if (program == nullptr || words == 0U) {
                return ESP_ERR_INVALID_ARG;
            }

            return ulp_process_macros_and_load(addr, program, &words);
        }

        template <typename T, std::size_t Extent>
            requires std::is_same_v<std::remove_cv_t<T>, Insn>
        [[nodiscard]] static esp_err_t macro(
            const std::uint32_t addr,
            const std::span<T, Extent> program,
            std::size_t* const words = nullptr) noexcept
        {
            auto count = program.size();
            const auto err = macro(addr, program.data(), count);
            if (words != nullptr) {
                *words = count;
            }
            return err;
        }

        template <typename T, std::size_t Extent>
            requires std::is_same_v<std::remove_cv_t<T>, Insn>
        [[nodiscard]] static esp_err_t macro(
            const std::span<T, Extent> program,
            std::size_t* const words = nullptr) noexcept
        {
            return macro(0U, program, words);
        }

        [[nodiscard]] static esp_err_t run(const std::uint32_t entry = 0U) noexcept
        {
            return ulp_run(entry);
        }

        static void stop() noexcept
        {
            ulp_timer_stop();
        }

        static void resume() noexcept
        {
            ulp_timer_resume();
        }

        [[nodiscard]] static esp_err_t isr(
            intr_handler_t handler,
            void* const arg = nullptr) noexcept
        {
            return ulp_isr_register(handler, arg);
        }

        [[nodiscard]] static esp_err_t off(
            intr_handler_t handler,
            void* const arg = nullptr) noexcept
        {
            return ulp_isr_deregister(handler, arg);
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
