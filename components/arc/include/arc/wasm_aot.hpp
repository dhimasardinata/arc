#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"
#include "arc/tee.hpp"

namespace arc::vm {

enum class WasmOp : std::uint8_t {
    end = 0x0bU,
    i32_const = 0x41U,
    i32_add = 0x6aU,
    i32_sub = 0x6bU,
    i32_mul = 0x6cU,
};

struct WasmAotConfig {
    std::uint32_t max_ops{128U};
    bool require_end{true};
};

struct WasmCursor {
    std::span<const std::uint8_t> bytes{};
    std::size_t pos{};

    [[nodiscard]] Result<std::uint8_t> u8() noexcept
    {
        if (pos >= bytes.size() || bytes.data() == nullptr) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        return bytes[pos++];
    }

    [[nodiscard]] Result<std::uint32_t> uleb() noexcept
    {
        std::uint32_t value{};
        auto shift = 0U;
        for (auto i = 0U; i < 5U; ++i) {
            auto byte = u8();
            if (!byte) {
                return fail(byte.error());
            }
            if (i == 4U && (*byte & 0xf0U) != 0U) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            value |= static_cast<std::uint32_t>(*byte & 0x7fU) << shift;
            if ((*byte & 0x80U) == 0U) {
                return value;
            }
            shift += 7U;
        }
        return fail(ESP_ERR_INVALID_ARG);
    }

    [[nodiscard]] Result<std::int32_t> sleb() noexcept
    {
        std::uint32_t value{};
        auto shift = 0U;
        std::uint8_t byte{};
        for (auto i = 0U; i < 5U; ++i) {
            auto next = u8();
            if (!next) {
                return fail(next.error());
            }
            byte = *next;
            value |= static_cast<std::uint32_t>(byte & 0x7fU) << shift;
            shift += 7U;
            if ((byte & 0x80U) == 0U) {
                if (shift < 32U && (byte & 0x40U) != 0U) {
                    value |= ~std::uint32_t{} << shift;
                }
                return std::bit_cast<std::int32_t>(value);
            }
        }
        return fail(ESP_ERR_INVALID_ARG);
    }
};

struct WasmSandbox {
    [[nodiscard]] static WorldRegion executable(const std::span<const std::uint32_t> image) noexcept
    {
        return {
            .base = image.data(),
            .bytes = image.size_bytes(),
            .owner = World::untrusted,
            .trusted = PmsAccess::read_execute,
            .untrusted = PmsAccess::read_execute,
        };
    }

    template <typename Policy>
    [[nodiscard]] static Status protect(const std::span<const std::uint32_t> image) noexcept
    {
        if (image.empty() || image.data() == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        const auto region = executable(image);
        return status(WorldGuard<Policy>::configure(std::span<const WorldRegion>{&region, 1U}));
    }
};

struct WasmAot {
    template <typename Policy>
    [[nodiscard]] static Result<std::span<std::uint32_t>> translate(
        const std::span<const std::uint8_t> wasm,
        const std::span<std::uint32_t> out,
        const WasmAotConfig config = {}) noexcept
    {
        if (wasm.empty() || out.empty() || wasm.data() == nullptr || out.data() == nullptr || config.max_ops == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto code = code_payload(wasm);
        if (!code) {
            return fail(code.error());
        }

        WasmCursor cursor{.bytes = *code};
        auto ops = std::uint32_t{};
        auto pos = std::size_t{};
        auto ended = false;
        while (cursor.pos < cursor.bytes.size()) {
            if (ops++ >= config.max_ops || pos >= out.size()) {
                return fail(ESP_ERR_NO_MEM);
            }
            auto op = cursor.u8();
            if (!op) {
                return fail(op.error());
            }
            switch (static_cast<WasmOp>(*op)) {
                case WasmOp::i32_const: {
                    auto imm = cursor.sleb();
                    if (!imm) {
                        return fail(imm.error());
                    }
                    out[pos++] = Policy::i32_const(*imm);
                    break;
                }
                case WasmOp::i32_add:
                    out[pos++] = Policy::i32_add();
                    break;
                case WasmOp::i32_sub:
                    out[pos++] = Policy::i32_sub();
                    break;
                case WasmOp::i32_mul:
                    out[pos++] = Policy::i32_mul();
                    break;
                case WasmOp::end:
                    out[pos++] = Policy::end();
                    ended = true;
                    cursor.pos = cursor.bytes.size();
                    break;
                default:
                    return fail(ESP_ERR_NOT_SUPPORTED);
            }
        }

        if (config.require_end && !ended) {
            return fail(ESP_ERR_INVALID_STATE);
        }
        const auto image = out.first(pos);
        if (const auto err = Policy::finalize(std::span<const std::uint32_t>{image}); err != ESP_OK) {
            return fail(err);
        }
        return image;
    }

private:
    [[nodiscard]] static Result<std::span<const std::uint8_t>> code_payload(const std::span<const std::uint8_t> wasm) noexcept
    {
        if (wasm.size() < 8U || wasm[0] != 0x00U || wasm[1] != 0x61U || wasm[2] != 0x73U || wasm[3] != 0x6dU) {
            return wasm;
        }
        if (wasm[4] != 0x01U || wasm[5] != 0x00U || wasm[6] != 0x00U || wasm[7] != 0x00U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        WasmCursor sections{.bytes = wasm, .pos = 8U};
        while (sections.pos < sections.bytes.size()) {
            auto id = sections.u8();
            auto size = sections.uleb();
            if (!id || !size || sections.pos + *size > sections.bytes.size()) {
                return fail(ESP_ERR_INVALID_ARG);
            }
            if (*id != 10U) {
                sections.pos += *size;
                continue;
            }

            WasmCursor code{.bytes = sections.bytes.subspan(sections.pos, *size)};
            auto functions = code.uleb();
            auto body_size = code.uleb();
            if (!functions || !body_size || *functions == 0U || code.pos + *body_size > code.bytes.size()) {
                return fail(ESP_ERR_INVALID_ARG);
            }

            const auto body_end = code.pos + *body_size;
            auto locals = code.uleb();
            if (!locals) {
                return fail(locals.error());
            }
            for (std::uint32_t local = 0; local < *locals; ++local) {
                auto count = code.uleb();
                auto type = code.u8();
                if (!count || !type) {
                    return fail(ESP_ERR_INVALID_ARG);
                }
            }
            return code.bytes.subspan(code.pos, body_end - code.pos);
        }
        return fail(ESP_ERR_NOT_FOUND);
    }
};

}  // namespace arc::vm
