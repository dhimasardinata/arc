#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "arc/result.hpp"
#include "arc/seq.hpp"

#if defined(__GNUC__)
#define ARC_LP_CORE [[gnu::section(".arc.lp_core.text"), gnu::used]]
#define ARC_LP_DATA [[gnu::section(".arc.lp_core.data"), gnu::used]]
#define ARC_LP_BSS [[gnu::section(".arc.lp_core.bss"), gnu::used]]
#define ARC_LP_SHARED [[gnu::section(".arc.lp_core.shared"), gnu::used]]
#else
#define ARC_LP_CORE
#define ARC_LP_DATA
#define ARC_LP_BSS
#define ARC_LP_SHARED
#endif

namespace arc::lp {

using Entry = void (*)() noexcept;

namespace detail {

[[nodiscard]] constexpr Status to_status(const Status value) noexcept
{
    return value;
}

[[nodiscard]] constexpr Status to_status(const esp_err_t value) noexcept
{
    return status(value);
}

}  // namespace detail

struct Image {
    std::span<const std::uint8_t> binary{};
    std::uintptr_t load{};
    Entry entry{};
    std::uint32_t stack{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return (binary.empty() || binary.data() != nullptr) &&
            !binary.empty() && entry != nullptr && stack != 0U;
    }
};

struct Control {
    Image image{};
    std::uint32_t period_us{};
    bool wake_main{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return image.valid() && period_us != 0U;
    }
};

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
struct Mailbox {
    static_assert(sizeof(T) > sizeof(std::uint32_t),
                  "[ARC ERROR] arc::lp::Mailbox is for multi-word payloads. Action: use arc::lp::Word for one word.");
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::lp::Mailbox payload must be trivially copyable. Action: use a flat struct.");
    static_assert(std::is_default_constructible_v<T>,
                  "[ARC ERROR] arc::lp::Mailbox payload must be default constructible. Action: add a zero state.");

    using value_type = T;

    SeqReg<T> to_lp{};
    SeqReg<T> to_host{};

    void host_write(const T& value) noexcept
    {
        to_lp.write(value);
    }

    [[nodiscard]] bool lp_read(T& out) const noexcept
    {
        return to_lp.try_read(out);
    }

    void lp_write(const T& value) noexcept
    {
        to_host.write(value);
    }

    [[nodiscard]] bool host_read(T& out) const noexcept
    {
        return to_host.try_read(out);
    }
};

template <typename Policy>
[[nodiscard]] Status load(
    Policy& policy,
    const Image image) noexcept
{
    if (!image.valid()) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    if constexpr (requires { policy.load(image); }) {
        return detail::to_status(policy.load(image));
    } else {
        static_cast<void>(policy);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy>
[[nodiscard]] Status start(
    Policy& policy,
    const Control control) noexcept
{
    if (!control.valid()) {
        return Status{fail(ESP_ERR_INVALID_ARG)};
    }

    if constexpr (requires { policy.start(control); }) {
        return detail::to_status(policy.start(control));
    } else {
        static_cast<void>(policy);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

template <typename Policy>
[[nodiscard]] Status stop(Policy& policy) noexcept
{
    if constexpr (requires { policy.stop(); }) {
        return detail::to_status(policy.stop());
    } else {
        static_cast<void>(policy);
        return Status{fail(ESP_ERR_NOT_SUPPORTED)};
    }
}

}  // namespace arc::lp
