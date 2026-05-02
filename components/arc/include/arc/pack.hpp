#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

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
        (write_one<Order, Fields>(out, pos, static_cast<Fields>(values)), ...);
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
        ((out = read_one<Order, Fields>(in, pos)), ...);
        return ok();
    }

private:
    template <Endian Order, typename Field>
    static void write_one(
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
    [[nodiscard]] static Field read_one(
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
};

}  // namespace arc::pack
