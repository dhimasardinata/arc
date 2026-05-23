#pragma once

#include <concepts>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "arc/soc/target.hpp"
#include "arc/stack.hpp"

namespace arc {

enum class Core : BaseType_t {
    core0 = 0,
    core1 = 1,
    any = tskNO_AFFINITY,
};

enum class CoreRole : std::uint8_t {
    ctrl,
    det,
};

template <Core Access, Core Owner>
concept CoreOwner = Access == Owner;

template <typename Target = soc::Target>
struct CoreMap {
    static constexpr bool dual = Target::cores > 1U;
    static constexpr Core ctrl = Core::core0;
    static constexpr Core det = dual ? Core::core1 : Core::core0;
    static constexpr bool simd = Target::simd;
    static constexpr bool riscv = Target::Arch::csr;
    static constexpr bool exp = Target::experimental;

    [[nodiscard]] static consteval Core core(const CoreRole role) noexcept
    {
        return role == CoreRole::det ? det : ctrl;
    }
};

[[nodiscard]] consteval std::size_t words(const std::size_t bytes) noexcept
{
    return bytes == 0U ? 0U : ((bytes - 1U) / sizeof(StackType_t)) + 1U;
}

template <auto* Object>
using StaticObject = std::remove_cv_t<std::remove_reference_t<decltype(*Object)>>;

template <auto* Object, typename T>
concept StaticTaskState =
    Object != nullptr &&
    std::same_as<StaticObject<Object>, T> &&
    !std::is_const_v<std::remove_pointer_t<decltype(Object)>>;

template <typename T, Core Owner>
class CoreLocal;

namespace detail {

template <typename Result>
inline constexpr bool scoped_core_result =
    std::is_void_v<Result> || (!std::is_reference_v<Result> && !std::is_pointer_v<Result>);

template <typename Fn, typename... Args>
consteval void require_scoped_core_result()
{
    using Result = std::invoke_result_t<Fn, Args...>;
    static_assert(scoped_core_result<Result>,
                  "[ARC ERROR] scoped core callback cannot return a reference or pointer. Action: copy out a value.");
}

}  // namespace detail

template <typename T, Core From, Core To>
class CoreMsg {
    static_assert(
        From != Core::any,
        "[ARC ERROR] arc::CoreMsg source must be a concrete core. Action: use Core::core0 or Core::core1.");
    static_assert(
        To != Core::any,
        "[ARC ERROR] arc::CoreMsg destination must be a concrete core. Action: use Core::core0 or Core::core1.");

public:
    using value_type = T;
    static constexpr Core from = From;
    static constexpr Core to = To;

    template <Core Access>
        requires CoreOwner<Access, To>
    [[nodiscard]] constexpr const T& get() const& noexcept
    {
        return value_;
    }

    [[nodiscard]] constexpr const T& get() const& noexcept
    {
        return get<To>();
    }

    template <Core Access>
        requires CoreOwner<Access, To> && std::copy_constructible<T>
    [[nodiscard]] constexpr T snapshot() const& noexcept(noexcept(T(value_)))
    {
        return value_;
    }

    [[nodiscard]] constexpr T snapshot() const& noexcept(noexcept(T(value_)))
        requires std::copy_constructible<T>
    {
        return snapshot<To>();
    }

    template <Core Access, typename Fn>
        requires CoreOwner<Access, To> && std::invocable<Fn, const T&>
    constexpr decltype(auto) with(Fn&& fn) const& noexcept(noexcept(std::invoke(std::forward<Fn>(fn), value_)))
    {
        detail::require_scoped_core_result<Fn, const T&>();
        return std::invoke(std::forward<Fn>(fn), value_);
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
    constexpr decltype(auto) with(Fn&& fn) const&
    {
        return with<To>(std::forward<Fn>(fn));
    }

private:
    template <typename, Core>
    friend class CoreLocal;

    constexpr explicit CoreMsg(const T& value) noexcept(noexcept(T(value)))
        requires std::copy_constructible<T>
        : value_(value)
    {}

    T value_;
};

template <typename T, Core Owner>
class CoreLocal {
    static_assert(
        Owner != Core::any,
        "[ARC ERROR] arc::CoreLocal owner must be a concrete core. Action: use Core::core0 or Core::core1.");

public:
    using value_type = T;
    static constexpr Core owner = Owner;

    template <Core To>
    using Msg = CoreMsg<T, Owner, To>;

    template <Core From>
    using Incoming = CoreMsg<T, From, Owner>;

    template <Core Access>
    [[nodiscard]] static consteval bool can_access() noexcept
    {
        return CoreOwner<Access, Owner>;
    }

    constexpr CoreLocal() noexcept(std::is_nothrow_default_constructible_v<T>)
        requires std::default_initializable<T>
    = default;

    constexpr explicit CoreLocal(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::move_constructible<T>
        : value_(std::move(value))
    {}

    template <Core Access>
        requires CoreOwner<Access, Owner>
    [[nodiscard]] constexpr T& get() & noexcept
    {
        return value_;
    }

    template <Core Access>
        requires CoreOwner<Access, Owner>
    [[nodiscard]] constexpr const T& get() const& noexcept
    {
        return value_;
    }

    template <Core Access>
        requires CoreOwner<Access, Owner> && std::copy_constructible<T>
    [[nodiscard]] constexpr T snapshot() const& noexcept(noexcept(T(value_)))
    {
        return value_;
    }

    [[nodiscard]] constexpr T snapshot() const& noexcept(noexcept(T(value_)))
        requires std::copy_constructible<T>
    {
        return snapshot<Owner>();
    }

    template <Core Access>
        requires CoreOwner<Access, Owner>
    [[nodiscard]] constexpr T&& get() && noexcept
    {
        return std::move(value_);
    }

    template <Core Access>
        requires CoreOwner<Access, Owner> && std::assignable_from<T&, T>
    constexpr void set(T value) noexcept(noexcept(std::declval<T&>() = std::move(value)))
    {
        value_ = std::move(value);
    }

    constexpr void set(T value) noexcept(noexcept(std::declval<T&>() = std::move(value)))
        requires std::assignable_from<T&, T>
    {
        set<Owner>(std::move(value));
    }

    template <Core Access, typename Fn>
        requires CoreOwner<Access, Owner> && std::invocable<Fn, T&>
    constexpr decltype(auto) with(Fn&& fn) & noexcept(noexcept(std::invoke(std::forward<Fn>(fn), value_)))
    {
        detail::require_scoped_core_result<Fn, T&>();
        return std::invoke(std::forward<Fn>(fn), value_);
    }

    template <typename Fn>
        requires std::invocable<Fn, T&>
    constexpr decltype(auto) with(Fn&& fn) &
    {
        return with<Owner>(std::forward<Fn>(fn));
    }

    template <Core Access, typename Fn>
        requires CoreOwner<Access, Owner> && std::invocable<Fn, const T&>
    constexpr decltype(auto) with(Fn&& fn) const& noexcept(noexcept(std::invoke(std::forward<Fn>(fn), value_)))
    {
        detail::require_scoped_core_result<Fn, const T&>();
        return std::invoke(std::forward<Fn>(fn), value_);
    }

    template <typename Fn>
        requires std::invocable<Fn, const T&>
    constexpr decltype(auto) with(Fn&& fn) const&
    {
        return with<Owner>(std::forward<Fn>(fn));
    }

    template <Core Access, Core To>
        requires CoreOwner<Access, Owner> && (To != Core::any) && std::copy_constructible<T>
    [[nodiscard]] constexpr Msg<To> msg() const noexcept(noexcept(Msg<To>{value_}))
    {
        return Msg<To>{value_};
    }

    template <Core To>
        requires(To != Core::any) && std::copy_constructible<T>
    [[nodiscard]] constexpr Msg<To> msg() const noexcept(noexcept(Msg<To>{value_}))
    {
        return msg<Owner, To>();
    }

    template <Core Access, Core From>
        requires CoreOwner<Access, Owner> && std::assignable_from<T&, const T&>
    constexpr void accept(const Incoming<From>& msg) noexcept(
        noexcept(std::declval<T&>() = msg.template get<Owner>()))
    {
        value_ = msg.template get<Owner>();
    }

    template <Core From>
        requires std::assignable_from<T&, const T&>
    constexpr void accept(const Incoming<From>& msg) noexcept(
        noexcept(std::declval<T&>() = msg.template get<Owner>()))
    {
        accept<Owner>(msg);
    }

private:
    T value_{};
};

template <typename T>
concept CoreMsgType = requires {
    typename T::value_type;
    { T::from } -> std::convertible_to<Core>;
    { T::to } -> std::convertible_to<Core>;
} && (T::from != Core::any) && (T::to != Core::any);

template <typename T>
concept CoreLocalType = requires {
    typename T::value_type;
    { T::owner } -> std::convertible_to<Core>;
} && (T::owner != Core::any);

template <std::size_t StackBytes, std::size_t RequiredBytes = stack::task_floor>
struct TaskMem {
    static_assert(StackBytes > 0, "stack size must be non-zero");
    static_assert(
        stack::fits<StackBytes, RequiredBytes>(),
        "static task stack is smaller than its compile-time Arc stack budget");
    static constexpr std::size_t depth = words(StackBytes);
    static constexpr std::size_t bytes = StackBytes;
    static constexpr std::size_t required = RequiredBytes;

    alignas(portBYTE_ALIGNMENT) std::array<StackType_t, depth> stack{};
    StaticTask_t tcb{};
};

template <std::size_t StackBytes, std::size_t RequiredBytes>
[[nodiscard]] inline TaskHandle_t spawn(
    TaskFunction_t entry,
    const char* name,
    void* context,
    const UBaseType_t priority,
    const Core core,
    TaskMem<StackBytes, RequiredBytes>& mem) noexcept
{
    return xTaskCreateStaticPinnedToCore(
        entry,
        name,
        static_cast<std::uint32_t>(StackBytes),
        context,
        priority,
        mem.stack.data(),
        &mem.tcb,
        static_cast<BaseType_t>(core));
}

[[noreturn]] inline void park_task() noexcept
{
    vTaskSuspend(nullptr);
    for (;;) {
        vTaskSuspend(nullptr);
    }
}

}  // namespace arc
