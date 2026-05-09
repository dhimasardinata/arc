#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc::ulp {

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
