#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/vm.hpp"

namespace arc::vm {

struct JitConfig {
    bool allow_memory{};
};

struct PseudoXtensaJitPolicy {
    [[nodiscard]] static constexpr std::uint32_t encode(
        const BpfOp op,
        const std::uint8_t dst,
        const std::uint8_t src,
        const std::int32_t imm) noexcept
    {
        return (static_cast<std::uint32_t>(op) << 24U) |
            (static_cast<std::uint32_t>(dst) << 20U) |
            (static_cast<std::uint32_t>(src) << 16U) |
            static_cast<std::uint32_t>(imm & 0xffff);
    }

    [[nodiscard]] static constexpr std::uint32_t exit() noexcept
    {
        return encode(BpfOp::exit, 0U, 0U, 0);
    }

    [[nodiscard]] static constexpr std::uint32_t alu(const BpfInsn insn) noexcept
    {
        return encode(insn.op, insn.dst(), insn.src(), insn.imm);
    }

    [[nodiscard]] static esp_err_t finalize(const std::span<const std::uint32_t>) noexcept
    {
        return ESP_OK;
    }
};

struct Jit {
    template <typename Policy = PseudoXtensaJitPolicy>
    [[nodiscard]] static Result<std::span<std::uint32_t>> translate(
        const std::span<const BpfInsn> program,
        const std::span<std::uint32_t> out,
        const JitConfig config = {}) noexcept
    {
        if (program.empty() || out.size() < program.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto pos = std::size_t{};
        for (const auto insn : program) {
            switch (insn.op) {
                case BpfOp::exit:
                    out[pos++] = Policy::exit();
                    break;
                case BpfOp::mov_imm:
                case BpfOp::mov_reg:
                case BpfOp::add_imm:
                case BpfOp::add_reg:
                case BpfOp::sub_imm:
                case BpfOp::sub_reg:
                case BpfOp::mul_imm:
                case BpfOp::mul_reg:
                case BpfOp::and_imm:
                case BpfOp::and_reg:
                case BpfOp::or_imm:
                case BpfOp::or_reg:
                case BpfOp::xor_imm:
                case BpfOp::xor_reg:
                case BpfOp::lsh_imm:
                case BpfOp::rsh_imm:
                    out[pos++] = Policy::alu(insn);
                    break;
                case BpfOp::load32:
                case BpfOp::store32:
                    if (!config.allow_memory) {
                        return fail(ESP_ERR_NOT_SUPPORTED);
                    }
                    out[pos++] = Policy::alu(insn);
                    break;
                default:
                    return fail(ESP_ERR_NOT_SUPPORTED);
            }
        }

        const auto image = out.first(pos);
        if (const auto err = Policy::finalize(std::span<const std::uint32_t>{image}); err != ESP_OK) {
            return fail(err);
        }
        return image;
    }
};

}  // namespace arc::vm
