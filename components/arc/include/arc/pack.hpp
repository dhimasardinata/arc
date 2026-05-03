#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>

#include "arc/result.hpp"

namespace arc::pack {

enum class Endian : std::uint8_t {
    big,
    little,
};

template <typename T>
concept Scalar = (std::is_integral_v<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>) &&
    !std::is_same_v<std::remove_cvref_t<T>, bool> &&
    sizeof(std::remove_cvref_t<T>) <= sizeof(std::uint64_t);

template <typename T, bool IsEnum = std::is_enum_v<std::remove_cvref_t<T>>>
struct WireType {
    using raw = std::remove_cvref_t<T>;
    using type = raw;
};

template <typename T>
struct WireType<T, true> {
    using type = std::underlying_type_t<std::remove_cvref_t<T>>;
};

template <typename T>
using WireTypeT = typename WireType<T>::type;

template <typename T>
using UnsignedWireT = std::make_unsigned_t<WireTypeT<T>>;

namespace detail {

template <Endian Order, typename Field>
static void write_scalar(
    const std::span<std::uint8_t> out,
    std::size_t& pos,
    const Field value) noexcept
{
    using U = UnsignedWireT<Field>;
    auto raw = static_cast<U>(static_cast<WireTypeT<Field>>(value));
    if constexpr ((Order == Endian::big && std::endian::native == std::endian::little) ||
                  (Order == Endian::little && std::endian::native == std::endian::big)) {
        raw = std::byteswap(raw);
    }
    std::memcpy(out.data() + pos, &raw, sizeof(U));
    pos += sizeof(U);
}

template <Endian Order, typename Field>
[[nodiscard]] static Field read_scalar(
    const std::span<const std::uint8_t> in,
    std::size_t& pos) noexcept
{
    using U = UnsignedWireT<Field>;
    U raw{};
    std::memcpy(&raw, in.data() + pos, sizeof(U));
    pos += sizeof(U);
    if constexpr ((Order == Endian::big && std::endian::native == std::endian::little) ||
                  (Order == Endian::little && std::endian::native == std::endian::big)) {
        raw = std::byteswap(raw);
    }
    if constexpr (std::is_enum_v<std::remove_cvref_t<Field>>) {
        return static_cast<Field>(static_cast<WireTypeT<Field>>(raw));
    } else {
        return static_cast<Field>(static_cast<WireTypeT<Field>>(raw));
    }
}

}  // namespace detail

template <typename... Fields>
struct Schema {
    static_assert((Scalar<Fields> && ...), "pack fields must be integer or enum scalars up to 64 bits");

    static constexpr std::size_t bytes = (std::size_t{} + ... + sizeof(WireTypeT<Fields>));

    template <Endian Order = Endian::big, typename... Values>
        requires(sizeof...(Values) == sizeof...(Fields) && (std::is_convertible_v<Values, Fields> && ...))
    [[nodiscard]] static Result<std::span<const std::uint8_t>> write(
        const std::span<std::uint8_t> out,
        Values... values) noexcept
    {
        if (out.size() < bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto pos = std::size_t{};
        (detail::write_scalar<Order, Fields>(out, pos, static_cast<Fields>(values)), ...);
        return out.first(bytes);
    }

    template <Endian Order = Endian::big, typename... Out>
        requires(sizeof...(Out) == sizeof...(Fields) && (std::is_lvalue_reference_v<Out&> && ...))
    [[nodiscard]] static Status read(
        const std::span<const std::uint8_t> in,
        Out&... out) noexcept
    {
        static_assert((std::is_assignable_v<Out&, Fields> && ...), "pack read outputs must accept schema fields");
        if (in.size() < bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto pos = std::size_t{};
        ((out = detail::read_scalar<Order, Fields>(in, pos)), ...);
        return ok();
    }
};

template <typename T>
struct MemberTraits;

template <typename Object, typename Type>
struct MemberTraits<Type Object::*> {
    using object = Object;
    using type = Type;
};

template <auto Member>
struct Field {
    static_assert(std::is_member_object_pointer_v<decltype(Member)>, "pack field must be a data member pointer");

    using Object = typename MemberTraits<decltype(Member)>::object;
    using Type = typename MemberTraits<decltype(Member)>::type;
    static_assert(Scalar<Type>, "pack struct fields must be integer or enum scalars up to 64 bits");

    static constexpr auto member = Member;
};

template <typename Object, typename... Fields>
struct Struct {
    static_assert(std::is_trivially_copyable_v<Object>, "pack struct object must be trivially copyable");
    static_assert((std::is_same_v<Object, typename Fields::Object> && ...), "pack fields must belong to object");

    static constexpr std::size_t bytes = (std::size_t{} + ... + sizeof(WireTypeT<typename Fields::Type>));

    template <Endian Order = Endian::big>
    [[nodiscard]] static Result<std::span<const std::uint8_t>> write(
        const std::span<std::uint8_t> out,
        const Object& value) noexcept
    {
        if (out.size() < bytes) {
            return fail(ESP_ERR_NO_MEM);
        }

        auto pos = std::size_t{};
        (write_member<Order, Fields>(out, pos, value), ...);
        return out.first(bytes);
    }

    template <Endian Order = Endian::big>
    [[nodiscard]] static Status read(
        const std::span<const std::uint8_t> in,
        Object& out) noexcept
    {
        if (in.size() < bytes) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto pos = std::size_t{};
        (read_member<Order, Fields>(in, pos, out), ...);
        return ok();
    }

private:
    template <Endian Order, typename Field>
    static void write_member(
        const std::span<std::uint8_t> out,
        std::size_t& pos,
        const Object& value) noexcept
    {
        detail::write_scalar<Order, typename Field::Type>(out, pos, value.*Field::member);
    }

    template <Endian Order, typename Field>
    static void read_member(
        const std::span<const std::uint8_t> in,
        std::size_t& pos,
        Object& out) noexcept
    {
        out.*Field::member = detail::read_scalar<Order, typename Field::Type>(in, pos);
    }
};

template <typename Object, auto... Members>
using StructOf = Struct<Object, Field<Members>...>;

template <typename Object>
struct ReflectMembers;

template <typename Object>
concept Reflected = requires {
    typename ReflectMembers<Object>::Codec;
};

template <Reflected Object>
using Reflect = typename ReflectMembers<Object>::Codec;

#define ARC_PACK_REFLECT(Type, ...)                           \
    template <>                                               \
    struct arc::pack::ReflectMembers<Type> {                  \
        using Codec = arc::pack::StructOf<Type, __VA_ARGS__>; \
    }

template <Endian Order = Endian::big, typename Codec, typename Object>
[[nodiscard]] Result<std::span<const std::uint8_t>> serialize(
    const std::span<std::uint8_t> out,
    const Object& value) noexcept
{
    return Codec::template write<Order>(out, value);
}

template <Endian Order = Endian::big, typename Codec, typename Object>
[[nodiscard]] Status deserialize(
    const std::span<const std::uint8_t> in,
    Object& out) noexcept
{
    return Codec::template read<Order>(in, out);
}

}  // namespace arc::pack
