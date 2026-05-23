#pragma once

#include <cstdint>
#include <type_traits>

#include "arc/result.hpp"

namespace arc {

template <typename T>
concept LockstepValue = std::is_trivially_copyable_v<T> && requires(const T& lhs, const T& rhs) {
    lhs == rhs;
};

template <typename T>
struct LockstepFault {
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::LockstepFault value must be trivially copyable. Action: use a flat output type.");

    T primary{};
    T replica{};
    std::uint32_t tick{};
};

template <typename T>
struct Lockstep {
    static_assert(std::is_trivially_copyable_v<T>,
                  "[ARC ERROR] arc::Lockstep output must be trivially copyable. Action: use a flat control output.");
    static_assert(LockstepValue<T>,
                  "[ARC ERROR] arc::Lockstep output must support ==. Action: add defaulted equality to the output type.");

    using Fault = LockstepFault<T>;

    template <typename Policy>
    [[nodiscard]] static Result<T> commit(
        const T& primary,
        const T& replica,
        const std::uint32_t tick) noexcept
    {
        if (primary == replica) {
            return ok(primary);
        }

        const Fault fault{
            .primary = primary,
            .replica = replica,
            .tick = tick,
        };
        capture<Policy>(fault);
        halt<Policy>(fault);
        return fail(ESP_ERR_INVALID_STATE);
    }

    [[nodiscard]] static constexpr bool match(const T& primary, const T& replica) noexcept
    {
        return primary == replica;
    }

private:
    template <typename Policy>
    static void capture(const Fault& fault) noexcept
    {
        if constexpr (requires { Policy::capture(fault); }) {
            Policy::capture(fault);
        }
    }

    template <typename Policy>
    static void halt(const Fault& fault) noexcept
    {
        if constexpr (requires { Policy::halt(fault); }) {
            static_cast<void>(Policy::halt(fault));
        } else if constexpr (requires { Policy::halt(); }) {
            static_cast<void>(Policy::halt());
        }
    }
};

}  // namespace arc
