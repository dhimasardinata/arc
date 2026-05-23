#pragma once

#include <functional>
#include <type_traits>

namespace arc::detail {

template <typename T>
struct IsReferenceWrapper : std::false_type {};

template <typename T>
struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type {};

template <typename T>
concept ReferenceWrapperResult = IsReferenceWrapper<std::remove_cvref_t<T>>::value;

template <typename T>
concept PlainScopedResult =
    !std::is_reference_v<T> && !std::is_pointer_v<T> && !ReferenceWrapperResult<T>;

}  // namespace arc::detail
