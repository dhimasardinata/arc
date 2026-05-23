#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "arc/task.hpp"

namespace arc {

enum class BorrowMode : std::uint8_t {
    read,
    mut,
};

template <Core Access, Core Owner>
concept CoreAccess = Owner == Core::any || CoreOwner<Access, Owner>;

template <auto* Object, Core Owner = Core::any>
struct StaticRef;

template <auto* Object, Core Owner, BorrowMode Mode>
class StaticLoan;

template <typename T>
concept StaticLoanType = requires {
    T::object;
    { T::owner } -> std::convertible_to<Core>;
    { T::mode } -> std::convertible_to<BorrowMode>;
};

template <auto* Object, Core Owner, BorrowMode Mode>
class StaticLoan {
    static_assert(Object != nullptr,
                  "[ARC ERROR] arc::StaticLoan needs static storage. Action: construct it through arc::StaticRef.");

    using raw_type = std::remove_reference_t<decltype(*Object)>;

public:
    using value_type = std::remove_cv_t<raw_type>;
    static constexpr Core owner = Owner;
    static constexpr BorrowMode mode = Mode;
    static constexpr auto object = Object;

    constexpr StaticLoan(const StaticLoan&) noexcept
        requires(Mode == BorrowMode::read)
    = default;

    constexpr StaticLoan& operator=(const StaticLoan&) noexcept
        requires(Mode == BorrowMode::read)
    = default;

    constexpr StaticLoan(const StaticLoan&) noexcept
        requires(Mode == BorrowMode::mut)
    = delete;

    constexpr StaticLoan& operator=(const StaticLoan&) noexcept
        requires(Mode == BorrowMode::mut)
    = delete;

    constexpr StaticLoan(StaticLoan&&) noexcept = default;
    constexpr StaticLoan& operator=(StaticLoan&&) noexcept = default;

    template <Core Access = Owner>
        requires CoreAccess<Access, Owner>
    [[nodiscard]] constexpr const value_type& get() const noexcept
    {
        return *Object;
    }

    template <Core Access = Owner>
        requires CoreAccess<Access, Owner> && (Mode == BorrowMode::mut) && (!std::is_const_v<raw_type>)
    [[nodiscard]] constexpr value_type& get() noexcept
    {
        return *Object;
    }

    [[nodiscard]] constexpr const value_type* operator->() const noexcept
    {
        return Object;
    }

    [[nodiscard]] constexpr const value_type& operator*() const noexcept
    {
        return *Object;
    }

    [[nodiscard]] constexpr value_type* operator->() noexcept
        requires(Mode == BorrowMode::mut) && (!std::is_const_v<raw_type>)
    {
        return Object;
    }

    [[nodiscard]] constexpr value_type& operator*() noexcept
        requires(Mode == BorrowMode::mut) && (!std::is_const_v<raw_type>)
    {
        return *Object;
    }

private:
    struct Key {};

    friend struct StaticRef<Object, Owner>;

    constexpr explicit StaticLoan(Key) noexcept {}
};

template <auto* Object, Core Owner>
using StaticRead = StaticLoan<Object, Owner, BorrowMode::read>;

template <auto* Object, Core Owner>
using StaticMut = StaticLoan<Object, Owner, BorrowMode::mut>;

namespace detail {

template <StaticLoanType A, StaticLoanType B>
[[nodiscard]] consteval bool same_object() noexcept
{
    if constexpr (std::same_as<decltype(A::object), decltype(B::object)>) {
        return A::object == B::object;
    }
    return false;
}

template <StaticLoanType A, StaticLoanType B>
[[nodiscard]] consteval bool loans_conflict() noexcept
{
    return same_object<A, B>() && (A::mode == BorrowMode::mut || B::mode == BorrowMode::mut);
}

template <StaticLoanType... Loans>
struct LoansOk;

template <>
struct LoansOk<> {
    static constexpr bool value = true;
};

template <StaticLoanType First, StaticLoanType... Rest>
struct LoansOk<First, Rest...> {
    static constexpr bool value = ((!loans_conflict<First, Rest>()) && ...) && LoansOk<Rest...>::value;
};

}  // namespace detail

template <StaticLoanType... Loans>
[[nodiscard]] consteval bool loans_ok() noexcept
{
    return detail::LoansOk<Loans...>::value;
}

template <StaticLoanType... Loans>
struct LoanPack {
    static_assert(loans_ok<Loans...>(),
                  "[ARC ERROR] arc::LoanPack has a static borrow conflict. Action: keep mutable static loans exclusive.");

    static constexpr std::size_t count = sizeof...(Loans);
};

template <auto* Object, Core Owner>
struct StaticRef {
    static_assert(Object != nullptr,
                  "[ARC ERROR] arc::StaticRef needs static storage. Action: pass the address of a global object.");

    using raw_type = std::remove_reference_t<decltype(*Object)>;
    using value_type = std::remove_cv_t<raw_type>;

    static_assert(!std::is_volatile_v<raw_type>,
                  "[ARC ERROR] arc::StaticRef cannot wrap volatile storage. Action: expose volatile access through a board policy.");

    static constexpr Core owner = Owner;
    static constexpr auto object = Object;

    template <Core Access = Owner>
        requires CoreAccess<Access, Owner>
    [[nodiscard]] static constexpr StaticLoan<Object, Owner, BorrowMode::read> read() noexcept
    {
        return StaticLoan<Object, Owner, BorrowMode::read>{typename StaticLoan<Object, Owner, BorrowMode::read>::Key{}};
    }

    template <Core Access = Owner>
        requires CoreAccess<Access, Owner> && (!std::is_const_v<raw_type>)
    [[nodiscard]] static constexpr StaticLoan<Object, Owner, BorrowMode::mut> write() noexcept
    {
        return StaticLoan<Object, Owner, BorrowMode::mut>{typename StaticLoan<Object, Owner, BorrowMode::mut>::Key{}};
    }
};

}  // namespace arc
