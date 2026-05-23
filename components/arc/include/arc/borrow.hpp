#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

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

template <typename T>
concept StaticRefType = requires {
    typename T::value_type;
    typename T::Read;
    typename T::Write;
    T::object;
    { T::owner } -> std::convertible_to<Core>;
    { T::writable } -> std::convertible_to<bool>;
};

template <auto* Object, Core Owner, BorrowMode Mode>
class StaticLoan {
    static_assert(Object != nullptr,
                  "[ARC ERROR] arc::StaticLoan needs static storage. Action: construct it through arc::StaticRef.");

    using raw_type = std::remove_reference_t<decltype(*Object)>;
    static_assert(Mode == BorrowMode::read || !std::is_const_v<raw_type>,
                  "[ARC ERROR] arc::StaticLoan mutable mode cannot wrap const storage. Action: use StaticRef::Read.");

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

    constexpr StaticLoan(StaticLoan&&) noexcept
        requires(Mode == BorrowMode::read)
    = default;

    constexpr StaticLoan& operator=(StaticLoan&&) noexcept
        requires(Mode == BorrowMode::read)
    = default;

    constexpr StaticLoan(StaticLoan&&) noexcept
        requires(Mode == BorrowMode::mut)
    = delete;

    constexpr StaticLoan& operator=(StaticLoan&&) noexcept
        requires(Mode == BorrowMode::mut)
    = delete;

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
        requires(Owner == Core::any)
    {
        return Object;
    }

    [[nodiscard]] constexpr const value_type& operator*() const noexcept
        requires(Owner == Core::any)
    {
        return *Object;
    }

    [[nodiscard]] constexpr value_type* operator->() noexcept
        requires(Owner == Core::any) && (Mode == BorrowMode::mut) && (!std::is_const_v<raw_type>)
    {
        return Object;
    }

    [[nodiscard]] constexpr value_type& operator*() noexcept
        requires(Owner == Core::any) && (Mode == BorrowMode::mut) && (!std::is_const_v<raw_type>)
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

template <auto* Object, StaticLoanType Loan>
[[nodiscard]] consteval bool loan_refs() noexcept
{
    using object_type = std::remove_cv_t<decltype(Object)>;
    using loan_object_type = std::remove_cv_t<decltype(Loan::object)>;
    if constexpr (std::same_as<object_type, loan_object_type>) {
        return Object == Loan::object;
    }
    return false;
}

template <Core Access, typename Loan>
concept LoanCoreAccess = StaticLoanType<Loan> && CoreAccess<Access, Loan::owner>;

template <typename Loan>
using LoanArg = std::conditional_t<Loan::mode == BorrowMode::mut,
                                   typename Loan::value_type&,
                                   const typename Loan::value_type&>;

template <Core Access, typename Loan>
    requires LoanCoreAccess<Access, Loan>
[[nodiscard]] constexpr LoanArg<Loan> loan_arg() noexcept
{
    if constexpr (Loan::mode == BorrowMode::mut) {
        return *Loan::object;
    } else {
        return static_cast<const typename Loan::value_type&>(*Loan::object);
    }
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

    template <Core Access>
    [[nodiscard]] static consteval bool can_access() noexcept
    {
        return (true && ... && detail::LoanCoreAccess<Access, Loans>);
    }

    template <Core Access, typename Fn>
        requires(can_access<Access>() && std::invocable<Fn, detail::LoanArg<Loans>...>)
    static constexpr decltype(auto) with(Fn&& fn) noexcept(
        noexcept(std::invoke(std::forward<Fn>(fn), detail::loan_arg<Access, Loans>()...)))
    {
        return std::invoke(std::forward<Fn>(fn), detail::loan_arg<Access, Loans>()...);
    }

    template <StaticLoanType Need>
    [[nodiscard]] static consteval bool contains() noexcept
    {
        return (false || ... || std::same_as<Need, Loans>);
    }

    template <auto* Object>
    [[nodiscard]] static consteval bool reads() noexcept
    {
        return (false || ... || detail::loan_refs<Object, Loans>());
    }

    template <StaticRefType Ref>
    [[nodiscard]] static consteval bool reads() noexcept
    {
        return reads<Ref::object>();
    }

    template <auto* Object>
    [[nodiscard]] static consteval bool writes() noexcept
    {
        return (false || ... || (detail::loan_refs<Object, Loans>() && Loans::mode == BorrowMode::mut));
    }

    template <StaticRefType Ref>
    [[nodiscard]] static consteval bool writes() noexcept
    {
        return writes<Ref::object>();
    }
};

template <typename T>
concept StaticLoanPack = requires {
    { T::count } -> std::convertible_to<std::size_t>;
};

template <typename Pack, typename Need>
concept HasLoan = StaticLoanPack<Pack> && StaticLoanType<Need> && Pack::template contains<Need>();

template <typename Pack, auto* Object>
concept HasStaticRead = StaticLoanPack<Pack> && Pack::template reads<Object>();

template <typename Pack, auto* Object>
concept HasStaticWrite = StaticLoanPack<Pack> && Pack::template writes<Object>();

template <typename Pack, typename Ref>
concept HasStaticRefRead = StaticLoanPack<Pack> && StaticRefType<Ref> && Pack::template reads<Ref>();

template <typename Pack, typename Ref>
concept HasStaticRefWrite = StaticLoanPack<Pack> && StaticRefType<Ref> && Pack::template writes<Ref>();

template <auto* Object, Core Owner>
struct StaticRef {
    static_assert(Object != nullptr,
                  "[ARC ERROR] arc::StaticRef needs static storage. Action: pass the address of a global object.");

    using raw_type = std::remove_reference_t<decltype(*Object)>;
    using value_type = std::remove_cv_t<raw_type>;
    using Read = StaticLoan<Object, Owner, BorrowMode::read>;
    using Write = StaticLoan<Object, Owner, BorrowMode::mut>;

    static_assert(!std::is_volatile_v<raw_type>,
                  "[ARC ERROR] arc::StaticRef cannot wrap volatile storage. Action: expose volatile access through a board policy.");

    static constexpr Core owner = Owner;
    static constexpr auto object = Object;
    static constexpr bool writable = !std::is_const_v<raw_type>;

    template <Core Access = Owner>
    [[nodiscard]] static consteval bool can_read() noexcept
    {
        return CoreAccess<Access, Owner>;
    }

    template <Core Access = Owner>
    [[nodiscard]] static consteval bool can_write() noexcept
    {
        return writable && CoreAccess<Access, Owner>;
    }

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

template <typename T, Core Access>
concept StaticReadable = StaticRefType<T> && requires {
    T::template read<Access>();
};

template <typename T, Core Access>
concept StaticWritable = StaticRefType<T> && requires {
    T::template write<Access>();
};

template <typename T, Core Access>
concept LoanReadable = StaticLoanType<T> && requires(const T& loan) {
    loan.template get<Access>();
};

template <typename T, Core Access>
concept LoanWritable = StaticLoanType<T> && requires(T& loan) {
    { loan.template get<Access>() } -> std::same_as<typename T::value_type&>;
};

template <StaticRefType... Refs>
using StaticReads = LoanPack<typename Refs::Read...>;

template <StaticRefType... Refs>
    requires((Refs::writable && ...))
using StaticWrites = LoanPack<typename Refs::Write...>;

namespace detail {

template <typename WriteRef, typename... ReadRefs>
concept StaticEditRefs = StaticRefType<WriteRef> && WriteRef::writable && (StaticRefType<ReadRefs> && ...);

}  // namespace detail

template <typename WriteRef, typename... ReadRefs>
    requires detail::StaticEditRefs<WriteRef, ReadRefs...>
using StaticEdit = LoanPack<typename WriteRef::Write, typename ReadRefs::Read...>;

template <StaticRefType Ref, Core Access = Ref::owner, typename Fn>
    requires StaticReadable<Ref, Access> && std::invocable<Fn, const typename Ref::value_type&>
constexpr decltype(auto) with_read(Fn&& fn)
{
    const auto loan = Ref::template read<Access>();
    return std::invoke(std::forward<Fn>(fn), loan.template get<Access>());
}

template <StaticRefType Ref, Core Access = Ref::owner, typename Fn>
    requires StaticWritable<Ref, Access> && std::invocable<Fn, typename Ref::value_type&>
constexpr decltype(auto) with_write(Fn&& fn)
{
    auto loan = Ref::template write<Access>();
    return std::invoke(std::forward<Fn>(fn), loan.template get<Access>());
}

template <Core Access, typename WriteRef, typename... ReadRefs, typename Fn>
    requires(detail::StaticEditRefs<WriteRef, ReadRefs...> &&
             StaticEdit<WriteRef, ReadRefs...>::template can_access<Access>() &&
             std::invocable<Fn, typename WriteRef::value_type&, const typename ReadRefs::value_type&...>)
constexpr decltype(auto) with_edit(Fn&& fn)
{
    return StaticEdit<WriteRef, ReadRefs...>::template with<Access>(std::forward<Fn>(fn));
}

}  // namespace arc
