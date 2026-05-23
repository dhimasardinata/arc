#pragma once

#include <functional>
#include <span>
#include <string_view>
#include <type_traits>

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
concept PlainScopedResult =
    !std::is_reference_v<T> && !std::is_pointer_v<T> && !ReferenceWrapperResult<T> &&
    !NonOwningViewResult<T>;

}  // namespace arc::detail
