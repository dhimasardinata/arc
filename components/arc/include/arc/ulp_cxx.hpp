#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::ulp {

enum class Op : std::uint8_t {
    read_adc,
    if_greater,
    wake_main,
    halt,
};

struct Step {
    Op op{};
    std::uint8_t arg{};
    std::uint16_t imm{};
};

struct IoStubPolicy {
    [[nodiscard]] static esp_err_t gpio_output(int) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static esp_err_t gpio_input(int) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    static void gpio_write(int, bool) noexcept {}

    [[nodiscard]] static bool gpio_read(int) noexcept
    {
        return false;
    }

    [[nodiscard]] static Result<std::uint16_t> adc_read(int) noexcept
    {
        return fail(ESP_ERR_INVALID_STATE);
    }

    [[nodiscard]] static esp_err_t i2c_write(
        int,
        std::uint8_t,
        std::span<const std::uint8_t>) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static esp_err_t i2c_read(
        int,
        std::uint8_t,
        std::span<std::uint8_t>) noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }

    [[nodiscard]] static esp_err_t wake_main() noexcept
    {
        return ESP_ERR_INVALID_STATE;
    }
};

template <std::size_t MaxSteps>
struct Program {
    static_assert(MaxSteps > 0U,
                  "[ARC ERROR] arc::ulp::Program needs storage. Action: set MaxSteps to the fixed ULP step budget.");

    std::array<Step, MaxSteps> steps{};
    std::size_t used{};
    bool ok{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return ok && used <= MaxSteps;
    }

    template <typename Policy = IoStubPolicy>
    [[nodiscard]] Status run() const noexcept
    {
        if (!valid()) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }

        std::uint16_t acc{};
        bool pass = true;
        for (std::size_t i = 0U; i < used; ++i) {
            const auto step = steps[i];
            switch (step.op) {
                case Op::read_adc: {
                    if (!pass) {
                        break;
                    }
                    const auto value = Policy::adc_read(step.arg);
                    if (!value) {
                        return Status{fail(value.error())};
                    }
                    acc = *value;
                    break;
                }
                case Op::if_greater:
                    pass = pass && acc > step.imm;
                    break;
                case Op::wake_main:
                    if (pass) {
                        return status(Policy::wake_main());
                    }
                    pass = true;
                    break;
                case Op::halt:
                    return ok_status();
            }
        }

        return ok_status();
    }

private:
    [[nodiscard]] static constexpr Status ok_status() noexcept
    {
        return {};
    }
};

template <std::size_t MaxSteps = 8U>
struct Builder {
    static_assert(MaxSteps > 0U,
                  "[ARC ERROR] arc::ulp::Builder needs storage. Action: set MaxSteps to the fixed ULP step budget.");

    std::array<Step, MaxSteps> steps{};
    std::size_t used{};
    bool ok{true};

    [[nodiscard]] constexpr Builder read_adc(const std::uint8_t channel) const noexcept
    {
        return emit({.op = Op::read_adc, .arg = channel});
    }

    [[nodiscard]] constexpr Builder if_greater(const std::uint16_t threshold) const noexcept
    {
        return emit({.op = Op::if_greater, .imm = threshold});
    }

    [[nodiscard]] constexpr Builder wake_main() const noexcept
    {
        return emit({.op = Op::wake_main});
    }

    [[nodiscard]] constexpr Builder halt() const noexcept
    {
        return emit({.op = Op::halt});
    }

    [[nodiscard]] constexpr Program<MaxSteps> build() const noexcept
    {
        return Program<MaxSteps>{
            .steps = steps,
            .used = used,
            .ok = ok,
        };
    }

private:
    [[nodiscard]] constexpr Builder emit(const Step step) const noexcept
    {
        auto out = *this;
        if (out.used >= MaxSteps) {
            out.ok = false;
            return out;
        }
        out.steps[out.used] = step;
        ++out.used;
        return out;
    }
};

template <int Pin, typename Policy = IoStubPolicy>
struct Gpio {
    [[nodiscard]] static Status output() noexcept
    {
        return status(Policy::gpio_output(Pin));
    }

    [[nodiscard]] static Status input() noexcept
    {
        return status(Policy::gpio_input(Pin));
    }

    static void write(const bool high) noexcept
    {
        Policy::gpio_write(Pin, high);
    }

    static void high() noexcept
    {
        write(true);
    }

    static void low() noexcept
    {
        write(false);
    }

    [[nodiscard]] static bool read() noexcept
    {
        return Policy::gpio_read(Pin);
    }
};

template <int Channel, typename Policy = IoStubPolicy>
struct Adc {
    [[nodiscard]] static Result<std::uint16_t> read_raw() noexcept
    {
        return Policy::adc_read(Channel);
    }
};

template <int Port, typename Policy = IoStubPolicy>
struct I2c {
    [[nodiscard]] static Status write(
        const std::uint8_t address,
        const std::span<const std::uint8_t> data) noexcept
    {
        return status(Policy::i2c_write(Port, address, data));
    }

    [[nodiscard]] static Status read(
        const std::uint8_t address,
        const std::span<std::uint8_t> data) noexcept
    {
        return status(Policy::i2c_read(Port, address, data));
    }
};

template <typename T>
concept SharedWriter = requires(T& shared, const typename T::value_type& value) {
    { shared.write(value) } -> std::same_as<void>;
};

template <typename Model, SharedWriter Shared, typename Policy = IoStubPolicy>
struct SleepFsm {
    using Input = typename Model::Input;
    using Output = typename Shared::value_type;

    [[nodiscard]] static Status run_once(Shared& shared, const Input& input) noexcept
    {
        const auto out = Model::eval(input);
        shared.write(out);
        if constexpr (requires { Model::wake(out); }) {
            if (Model::wake(out)) {
                return status(Policy::wake_main());
            }
        }
        return ok();
    }
};

}  // namespace arc::ulp
