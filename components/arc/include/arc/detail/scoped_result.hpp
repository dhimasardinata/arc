#pragma once

#include <array>
#include <cstddef>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

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
struct NoExtraScopedUnsafe : std::false_type {};

template <template <typename> typename ExtraUnsafe, typename T>
struct IsSafeScopedResult : std::bool_constant<!std::is_reference_v<T> &&
                                               !std::is_pointer_v<std::remove_cv_t<T>> &&
                                               !ReferenceWrapperResult<T> &&
                                               !NonOwningViewResult<T> &&
                                               !ExtraUnsafe<std::remove_cvref_t<T>>::value> {};

template <template <typename> typename ExtraUnsafe, typename First, typename Second>
struct IsSafeScopedResult<ExtraUnsafe, std::pair<First, Second>>
    : std::bool_constant<IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<First>>::value &&
                         IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Second>>::value> {};

template <template <typename> typename ExtraUnsafe, typename... Items>
struct IsSafeScopedResult<ExtraUnsafe, std::tuple<Items...>>
    : std::bool_constant<(IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Items>>::value && ...)> {};

template <template <typename> typename ExtraUnsafe, typename Item, std::size_t Count>
struct IsSafeScopedResult<ExtraUnsafe, std::array<Item, Count>>
    : IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Item>> {};

template <template <typename> typename ExtraUnsafe, typename Item>
struct IsSafeScopedResult<ExtraUnsafe, std::optional<Item>>
    : IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Item>> {};

template <template <typename> typename ExtraUnsafe, typename... Items>
struct IsSafeScopedResult<ExtraUnsafe, std::variant<Items...>>
    : std::bool_constant<(IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Items>>::value && ...)> {};

template <template <typename> typename ExtraUnsafe, typename Value, typename Error>
struct IsSafeScopedResult<ExtraUnsafe, std::expected<Value, Error>>
    : std::bool_constant<IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Value>>::value &&
                         IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<Error>>::value> {};

template <typename T>
struct IsPlainScopedResult : IsSafeScopedResult<NoExtraScopedUnsafe, T> {};

template <template <typename> typename ExtraUnsafe, typename T>
concept SafeScopedResult = IsSafeScopedResult<ExtraUnsafe, std::remove_cv_t<T>>::value;

template <typename T>
concept PlainScopedResult = IsPlainScopedResult<std::remove_cv_t<T>>::value;

}  // namespace arc::detail
