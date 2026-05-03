#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace arc::ulp::riscv {

enum class Reg : std::uint32_t {
    x0 = 0U,
    x1 = 1U,
    x2 = 2U,
    x3 = 3U,
    x4 = 4U,
    x5 = 5U,
    x6 = 6U,
    x7 = 7U,
};

[[nodiscard]] consteval std::uint32_t encode_i(
    const std::uint32_t imm,
    const Reg rs1,
    const std::uint32_t funct3,
    const Reg rd,
    const std::uint32_t opcode) noexcept
{
    return ((imm & 0xfffU) << 20U) |
        (static_cast<std::uint32_t>(rs1) << 15U) |
        ((funct3 & 0x7U) << 12U) |
        (static_cast<std::uint32_t>(rd) << 7U) |
        (opcode & 0x7fU);
}

[[nodiscard]] consteval std::uint32_t encode_s(
    const std::uint32_t imm,
    const Reg rs2,
    const Reg rs1,
    const std::uint32_t funct3,
    const std::uint32_t opcode) noexcept
{
    return (((imm >> 5U) & 0x7fU) << 25U) |
        (static_cast<std::uint32_t>(rs2) << 20U) |
        (static_cast<std::uint32_t>(rs1) << 15U) |
        ((funct3 & 0x7U) << 12U) |
        ((imm & 0x1fU) << 7U) |
        (opcode & 0x7fU);
}

[[nodiscard]] consteval std::uint32_t addi(
    const Reg rd,
    const Reg rs1,
    const std::int32_t imm) noexcept
{
    return encode_i(static_cast<std::uint32_t>(imm), rs1, 0U, rd, 0x13U);
}

[[nodiscard]] consteval std::uint32_t ADDI(
    const Reg rd,
    const Reg rs1,
    const std::int32_t imm) noexcept
{
    return addi(rd, rs1, imm);
}

[[nodiscard]] consteval std::uint32_t sw(
    const Reg rs2,
    const Reg rs1,
    const std::int32_t imm) noexcept
{
    return encode_s(static_cast<std::uint32_t>(imm), rs2, rs1, 2U, 0x23U);
}

[[nodiscard]] consteval std::uint32_t SW(
    const Reg rs2,
    const Reg rs1,
    const std::int32_t imm) noexcept
{
    return sw(rs2, rs1, imm);
}

[[nodiscard]] consteval std::uint32_t halt() noexcept
{
    return 0x0010'0073U;
}

[[nodiscard]] consteval std::uint32_t HALT() noexcept
{
    return halt();
}

template <std::uint32_t... Words>
[[nodiscard]] consteval auto assemble() noexcept
{
    return std::array<std::uint32_t, sizeof...(Words)>{Words...};
}

template <typename... Words>
[[nodiscard]] consteval auto assemble(Words... words) noexcept
{
    return std::array<std::uint32_t, sizeof...(Words)>{static_cast<std::uint32_t>(words)...};
}

}  // namespace arc::ulp::riscv
