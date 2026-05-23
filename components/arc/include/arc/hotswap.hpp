#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "arc/hotpatch.hpp"
#include "arc/jit.hpp"
#include "arc/result.hpp"
#include "arc/wasm_aot.hpp"

namespace arc::vm {

enum class HotSwapKind : std::uint8_t {
    native,
    bpf,
    wasm,
};

struct HotSwapPlan {
    HotSwapKind kind{};
    std::uint32_t version{};
    std::span<const std::uint8_t> payload{};
    std::span<const std::uint8_t> signature{};
    std::size_t entry_offset{};
};

struct HotSwapImage {
    HotSwapKind kind{};
    std::uint32_t version{};
    std::span<const std::uint32_t> words{};
    const void* entry{};
};

struct HotSwapGate {
    HotSwapKind kind{};
    std::uint32_t active{};
    std::uint32_t staged{};
    bool has_active{};
    bool has_staged{};

    [[nodiscard]] Status accept(const HotSwapPlan plan) noexcept
    {
        if (!valid(plan.payload) || !valid(plan.signature) || plan.version == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (has_active && plan.version <= active) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        kind = plan.kind;
        staged = plan.version;
        has_staged = true;
        return ok();
    }

    [[nodiscard]] Status commit(const HotSwapImage image) noexcept
    {
        if (!valid(image.words) || image.entry == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (!has_staged || image.kind != kind || image.version != staged) {
            return Status{fail(ESP_ERR_INVALID_STATE)};
        }
        active = image.version;
        has_active = true;
        has_staged = false;
        return ok();
    }

    constexpr void cancel() noexcept
    {
        has_staged = false;
        staged = 0U;
    }

private:
    template <typename T>
    [[nodiscard]] static constexpr bool valid(const std::span<T> span) noexcept
    {
        return !span.empty() && span.data() != nullptr;
    }
};

struct HotSwap {
    template <typename Policy>
    [[nodiscard]] static Status verify(const HotSwapPlan plan) noexcept
    {
        if (!valid(plan.payload) || !valid(plan.signature) || plan.version == 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if constexpr (requires { Policy::verify(plan); }) {
            return status(Policy::verify(plan));
        } else if constexpr (requires { Policy::verify(plan.payload, plan.signature); }) {
            return status(Policy::verify(plan.payload, plan.signature));
        } else {
            return Status{fail(ESP_ERR_NOT_SUPPORTED)};
        }
    }

    template <typename Policy, typename HeapPolicy = HotPatchHeapPolicy>
    [[nodiscard]] static Result<HotPatchImage<HeapPolicy>> native(const HotSwapPlan plan) noexcept
    {
        if (plan.kind != HotSwapKind::native) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        ARC_CHECK(verify<Policy>(plan));

        auto image = HotPatch::load<HeapPolicy>(plan.payload, plan.entry_offset);
        if (!image) {
            return fail(image.error());
        }
        if constexpr (requires { Policy::protect(image->block); }) {
            if (const auto err = Policy::protect(image->block); err != ESP_OK) {
                return fail(err);
            }
        } else {
            return fail(ESP_ERR_NOT_SUPPORTED);
        }
        return *std::move(image);
    }

    template <typename Policy, typename JitPolicy = PseudoXtensaJitPolicy>
    [[nodiscard]] static Result<HotSwapImage> bpf(
        const HotSwapPlan plan,
        const std::span<BpfInsn> decoded,
        const std::span<std::uint32_t> out,
        const JitConfig config = {}) noexcept
    {
        if (plan.kind != HotSwapKind::bpf) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        ARC_CHECK(verify<Policy>(plan));

        auto program = BPF<>::decode(plan.payload, decoded);
        if (!program) {
            return fail(program.error());
        }
        auto image = Jit::translate<JitPolicy>(*program, out, config);
        if (!image) {
            return fail(image.error());
        }

        HotSwapImage staged{
            .kind = plan.kind,
            .version = plan.version,
            .words = *image,
            .entry = image->data(),
        };
        ARC_CHECK(protect<Policy>(staged));
        return staged;
    }

    template <typename Policy, typename AotPolicy>
    [[nodiscard]] static Result<HotSwapImage> wasm(
        const HotSwapPlan plan,
        const std::span<std::uint32_t> out,
        const WasmAotConfig config = {}) noexcept
    {
        if (plan.kind != HotSwapKind::wasm) {
            return fail(ESP_ERR_INVALID_ARG);
        }
        ARC_CHECK(verify<Policy>(plan));

        auto image = WasmAot::translate<AotPolicy>(plan.payload, out, config);
        if (!image) {
            return fail(image.error());
        }

        HotSwapImage staged{
            .kind = plan.kind,
            .version = plan.version,
            .words = *image,
            .entry = image->data(),
        };
        ARC_CHECK(protect<Policy>(staged));
        return staged;
    }

    template <typename Policy>
    [[nodiscard]] static Status protect(const HotSwapImage image) noexcept
    {
        if (!valid(image.words) || image.entry == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if constexpr (requires { Policy::protect(image); }) {
            return status(Policy::protect(image));
        } else {
            return Status{fail(ESP_ERR_NOT_SUPPORTED)};
        }
    }

    template <typename Policy>
    [[nodiscard]] static Status activate(const HotSwapImage image) noexcept
    {
        if (!valid(image.words) || image.entry == nullptr) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if constexpr (requires { Policy::activate(image); }) {
            return status(Policy::activate(image));
        } else {
            return Status{fail(ESP_ERR_NOT_SUPPORTED)};
        }
    }

private:
    template <typename T>
    [[nodiscard]] static constexpr bool valid(const std::span<T> span) noexcept
    {
        return !span.empty() && span.data() != nullptr;
    }
};

}  // namespace arc::vm
