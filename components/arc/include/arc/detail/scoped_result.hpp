#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace arc::detail {

template <typename T>
struct IsReferenceWrapper : std::false_type {};

template <typename T>
struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type {};

template <typename T>
concept ReferenceWrapperResult = IsReferenceWrapper<std::remove_cvref_t<T>>::value;

template <typename T>
struct IsSpan : std::false_type {};

template <typename T, std::size_t Extent>
struct IsSpan<std::span<T, Extent>> : std::true_type {};

template <typename T>
concept SpanResult = IsSpan<std::remove_cvref_t<T>>::value;

template <typename T>
struct IsStringView : std::false_type {};

template <typename Char, typename Traits>
struct IsStringView<std::basic_string_view<Char, Traits>> : std::true_type {};

template <typename T>
concept StringViewResult = IsStringView<std::remove_cvref_t<T>>::value;

template <typename T>
concept NonOwningViewResult = SpanResult<T> || StringViewResult<T>;

template <typename T>
struct IsPlainScopedResult : std::bool_constant<!std::is_reference_v<T> &&
                                                !std::is_pointer_v<std::remove_cv_t<T>> &&
                                                !ReferenceWrapperResult<T> &&
                                                !NonOwningViewResult<T>> {};

template <typename First, typename Second>
struct IsPlainScopedResult<std::pair<First, Second>>
    : std::bool_constant<IsPlainScopedResult<std::remove_cv_t<First>>::value &&
                         IsPlainScopedResult<std::remove_cv_t<Second>>::value> {};

template <typename... Items>
struct IsPlainScopedResult<std::tuple<Items...>>
    : std::bool_constant<(IsPlainScopedResult<std::remove_cv_t<Items>>::value && ...)> {};

template <typename Item, std::size_t Count>
struct IsPlainScopedResult<std::array<Item, Count>> : IsPlainScopedResult<std::remove_cv_t<Item>> {};

template <typename T>
concept PlainScopedResult = IsPlainScopedResult<std::remove_cv_t<T>>::value;

}  // namespace arc::detail
