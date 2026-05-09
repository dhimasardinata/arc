#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

#include "arc/result.hpp"
#include "arc/tee.hpp"

namespace arc::vm {

enum class BpfOp : std::uint8_t {
    exit = 0x00U,
    mov_imm = 0x01U,
    mov_reg = 0x02U,
    add_imm = 0x03U,
    add_reg = 0x04U,
    sub_imm = 0x05U,
    sub_reg = 0x06U,
    mul_imm = 0x07U,
    mul_reg = 0x08U,
    div_imm = 0x09U,
    div_reg = 0x0aU,
    and_imm = 0x0bU,
    and_reg = 0x0cU,
    or_imm = 0x0dU,
    or_reg = 0x0eU,
    xor_imm = 0x0fU,
    xor_reg = 0x10U,
    lsh_imm = 0x11U,
    rsh_imm = 0x12U,
    load32 = 0x20U,
    store32 = 0x21U,
    ja = 0x30U,
    jeq_imm = 0x31U,
    jne_imm = 0x32U,
    jgt_imm = 0x33U,
    jge_imm = 0x34U,
    jset_imm = 0x35U,
};

struct BpfInsn {
    BpfOp op{};
    std::uint8_t regs{};
    std::int16_t offset{};
    std::int32_t imm{};

    [[nodiscard]] static constexpr BpfInsn make(
        const BpfOp op,
        const std::uint8_t dst = 0U,
        const std::uint8_t src = 0U,
        const std::int16_t offset = 0,
        const std::int32_t imm = 0) noexcept
    {
        return {
            .op = op,
            .regs = static_cast<std::uint8_t>((dst & 0x0fU) | ((src & 0x0fU) << 4U)),
            .offset = offset,
            .imm = imm,
        };
    }

    [[nodiscard]] constexpr std::uint8_t dst() const noexcept
    {
        return regs & 0x0fU;
    }

    [[nodiscard]] constexpr std::uint8_t src() const noexcept
    {
        return regs >> 4U;
    }
};

static_assert(sizeof(BpfInsn) == 8U);
static_assert(std::is_trivially_copyable_v<BpfInsn>);

struct BpfLimits {
    std::uint32_t max_steps{1024U};
    bool writable_memory{true};
};

struct BpfResult {
    std::int64_t value{};
    std::uint32_t steps{};
};

struct BpfSandbox {
    [[nodiscard]] static WorldRegion world(
        const std::span<std::uint8_t> buffer,
        const PmsAccess untrusted = PmsAccess::read) noexcept
    {
        return {
            .base = buffer.data(),
            .bytes = buffer.size(),
            .owner = World::untrusted,
            .trusted = PmsAccess::read_write,
            .untrusted = untrusted,
        };
    }

    template <typename Policy>
    [[nodiscard]] static Status protect(
        const std::span<std::uint8_t> buffer,
        const PmsAccess untrusted = PmsAccess::read) noexcept
    {
        const auto region = world(buffer, untrusted);
        return status(WorldGuard<Policy>::configure(std::span<const WorldRegion>{&region, 1U}));
    }
};

template <std::size_t Registers = 11U>
struct BPF {
    static_assert(Registers >= 2U, "BPF VM needs at least r0 and one argument register");

    std::array<std::int64_t, Registers> regs{};

    static constexpr auto instruction_bytes = sizeof(BpfInsn);

    [[nodiscard]] static Result<std::span<BpfInsn>> decode(
        const std::span<const std::uint8_t> bytes,
        const std::span<BpfInsn> out) noexcept
    {
        if (bytes.empty() || (bytes.size() % instruction_bytes) != 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        const auto count = bytes.size() / instruction_bytes;
        if (out.size() < count) {
            return fail(ESP_ERR_NO_MEM);
        }
        std::memcpy(out.data(), bytes.data(), bytes.size());
        return out.first(count);
    }

    [[nodiscard]] Result<BpfResult> run(
        const std::span<const BpfInsn> program,
        const std::span<std::uint8_t> memory = {},
        const std::span<const std::int64_t> args = {},
        const BpfLimits limits = {}) noexcept
    {
        if (program.empty() || limits.max_steps == 0U || args.size() + 1U > Registers) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        regs.fill(0);
        for (std::size_t i = 0; i < args.size(); ++i) {
            regs[i + 1U] = args[i];
        }
        regs[Registers - 1U] = static_cast<std::int64_t>(memory.size());

        auto pc = std::int64_t{};
        for (std::uint32_t step = 0; step < limits.max_steps; ++step) {
            if (pc < 0 || static_cast<std::uint64_t>(pc) >= program.size()) {
                return fail(ESP_ERR_INVALID_STATE);
            }

            const auto insn = program[static_cast<std::size_t>(pc)];
            if (insn.dst() >= Registers || insn.src() >= Registers) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            switch (insn.op) {
                case BpfOp::exit:
                    return BpfResult{.value = regs[0], .steps = step + 1U};
                case BpfOp::mov_imm:
                    regs[insn.dst()] = insn.imm;
                    ++pc;
                    break;
                case BpfOp::mov_reg:
                    regs[insn.dst()] = regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::add_imm:
                    regs[insn.dst()] += insn.imm;
                    ++pc;
                    break;
                case BpfOp::add_reg:
                    regs[insn.dst()] += regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::sub_imm:
                    regs[insn.dst()] -= insn.imm;
                    ++pc;
                    break;
                case BpfOp::sub_reg:
                    regs[insn.dst()] -= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::mul_imm:
                    regs[insn.dst()] *= insn.imm;
                    ++pc;
                    break;
                case BpfOp::mul_reg:
                    regs[insn.dst()] *= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::div_imm:
                    if (insn.imm == 0) {
                        return fail(ESP_ERR_INVALID_ARG);
                    }
                    regs[insn.dst()] /= insn.imm;
                    ++pc;
                    break;
                case BpfOp::div_reg:
                    if (regs[insn.src()] == 0) {
                        return fail(ESP_ERR_INVALID_ARG);
                    }
                    regs[insn.dst()] /= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::and_imm:
                    regs[insn.dst()] &= insn.imm;
                    ++pc;
                    break;
                case BpfOp::and_reg:
                    regs[insn.dst()] &= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::or_imm:
                    regs[insn.dst()] |= insn.imm;
                    ++pc;
                    break;
                case BpfOp::or_reg:
                    regs[insn.dst()] |= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::xor_imm:
                    regs[insn.dst()] ^= insn.imm;
                    ++pc;
                    break;
                case BpfOp::xor_reg:
                    regs[insn.dst()] ^= regs[insn.src()];
                    ++pc;
                    break;
                case BpfOp::lsh_imm:
                    if (insn.imm < 0 || insn.imm >= 63) {
                        return fail(ESP_ERR_INVALID_ARG);
                    }
                    regs[insn.dst()] <<= insn.imm;
                    ++pc;
                    break;
                case BpfOp::rsh_imm:
                    if (insn.imm < 0 || insn.imm >= 63) {
                        return fail(ESP_ERR_INVALID_ARG);
                    }
                    regs[insn.dst()] = static_cast<std::int64_t>(static_cast<std::uint64_t>(regs[insn.dst()]) >> insn.imm);
                    ++pc;
                    break;
                case BpfOp::load32: {
                    const auto read = read_u32(memory, address(insn, insn.src()));
                    if (!read) {
                        return fail(read.error());
                    }
                    regs[insn.dst()] = *read;
                    ++pc;
                    break;
                }
                case BpfOp::store32:
                    if (!limits.writable_memory) {
                        return fail(ESP_ERR_INVALID_STATE);
                    }
                    if (const auto written = write_u32(memory, address(insn, insn.dst()), static_cast<std::uint32_t>(regs[insn.src()])); !written) {
                        return fail(written.error());
                    }
                    ++pc;
                    break;
                case BpfOp::ja:
                    pc = jump(pc, insn.offset);
                    break;
                case BpfOp::jeq_imm:
                    pc = regs[insn.dst()] == insn.imm ? jump(pc, insn.offset) : pc + 1;
                    break;
                case BpfOp::jne_imm:
                    pc = regs[insn.dst()] != insn.imm ? jump(pc, insn.offset) : pc + 1;
                    break;
                case BpfOp::jgt_imm:
                    pc = regs[insn.dst()] > insn.imm ? jump(pc, insn.offset) : pc + 1;
                    break;
                case BpfOp::jge_imm:
                    pc = regs[insn.dst()] >= insn.imm ? jump(pc, insn.offset) : pc + 1;
                    break;
                case BpfOp::jset_imm:
                    pc = (regs[insn.dst()] & insn.imm) != 0 ? jump(pc, insn.offset) : pc + 1;
                    break;
                default:
                    return fail(ESP_ERR_INVALID_STATE);
            }
        }
        return fail(ESP_ERR_INVALID_STATE);
    }

private:
    [[nodiscard]] std::int64_t address(
        const BpfInsn insn,
        const std::uint8_t reg) const noexcept
    {
        return regs[reg] + insn.offset + insn.imm;
    }

    [[nodiscard]] static constexpr std::int64_t jump(
        const std::int64_t pc,
        const std::int16_t offset) noexcept
    {
        return pc + 1 + offset;
    }

    [[nodiscard]] static Result<std::uint32_t> read_u32(
        const std::span<const std::uint8_t> memory,
        const std::int64_t address) noexcept
    {
        if (address < 0 || static_cast<std::uint64_t>(address) + sizeof(std::uint32_t) > memory.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        std::uint32_t value{};
        std::memcpy(&value, memory.data() + address, sizeof(value));
        return value;
    }

    [[nodiscard]] static Status write_u32(
        const std::span<std::uint8_t> memory,
        const std::int64_t address,
        const std::uint32_t value) noexcept
    {
        if (address < 0 || static_cast<std::uint64_t>(address) + sizeof(std::uint32_t) > memory.size()) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        std::memcpy(memory.data() + address, &value, sizeof(value));
        return ok();
    }
};

}  // namespace arc::vm
