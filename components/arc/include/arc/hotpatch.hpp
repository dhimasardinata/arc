#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "esp_heap_caps.h"

#include "arc/cache_lock.hpp"
#include "arc/mask.hpp"
#include "arc/result.hpp"

namespace arc {

struct HotPatchBlock {
    void* memory{};
    std::size_t bytes{};
    void* entry{};
};

struct HotPatchDetour {
    void* target{};
    const void* replacement{};
    std::uint32_t instruction{};
};

struct HotPatchHeapPolicy {
    static constexpr std::uint32_t exec_caps =
#if defined(MALLOC_CAP_EXEC)
        MALLOC_CAP_EXEC | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
#else
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
#endif

    [[nodiscard]] static void* allocate(const std::size_t bytes) noexcept
    {
        return heap_caps_malloc(bytes, exec_caps);
    }

    static void release(void* const pointer) noexcept
    {
        heap_caps_free(pointer);
    }
};

template <typename HeapPolicy = HotPatchHeapPolicy>
struct HotPatchImage {
    HotPatchBlock block{};

    HotPatchImage() = default;
    HotPatchImage(const HotPatchImage&) = delete;
    HotPatchImage& operator=(const HotPatchImage&) = delete;

    HotPatchImage(HotPatchImage&& other) noexcept
        : block{other.block}
    {
        other.block = {};
    }

    HotPatchImage& operator=(HotPatchImage&& other) noexcept
    {
        if (this != &other) {
            reset();
            block = other.block;
            other.block = {};
        }
        return *this;
    }

    ~HotPatchImage()
    {
        reset();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return block.memory != nullptr;
    }

    [[nodiscard]] void* entry() const noexcept
    {
        return block.entry;
    }

    void reset() noexcept
    {
        if (block.memory != nullptr) {
            HeapPolicy::release(block.memory);
            block = {};
        }
    }
};

struct HotPatch {
    template <typename HeapPolicy = HotPatchHeapPolicy>
    [[nodiscard]] static Result<HotPatchImage<HeapPolicy>> load(
        const std::span<const std::uint8_t> payload,
        const std::size_t entry_offset = 0U) noexcept
    {
        if (payload.empty() || entry_offset >= payload.size()) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        auto* const memory = HeapPolicy::allocate(payload.size());
        if (memory == nullptr) {
            return fail(ESP_ERR_NO_MEM);
        }

        std::memcpy(memory, payload.data(), payload.size());
        if constexpr (requires { HeapPolicy::sync_code(memory, payload.size()); }) {
            if (const auto err = HeapPolicy::sync_code(memory, payload.size()); err != ESP_OK) {
                HeapPolicy::release(memory);
                return fail(err);
            }
        }

        HotPatchImage<HeapPolicy> image{};
        image.block = {
            .memory = memory,
            .bytes = payload.size(),
            .entry = static_cast<std::uint8_t*>(memory) + entry_offset,
        };
        return image;
    }

    template <typename PatchPolicy, typename Guard = Silence>
    [[nodiscard]] static Status install(const HotPatchDetour detour) noexcept
    {
        if (detour.target == nullptr || detour.replacement == nullptr || detour.instruction == 0U ||
            (reinterpret_cast<std::uintptr_t>(detour.target) & (alignof(std::uint32_t) - 1U)) != 0U) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        const CacheRegion target_region{.address = detour.target, .bytes = sizeof(std::uint32_t)};
        if constexpr (requires { PatchPolicy::lock_code(target_region); }) {
            CacheLock<PatchPolicy> cache{};
            if (const auto err = cache.lock_code(target_region); err != ESP_OK) {
                return Status{fail(err)};
            }
        }

        auto patch_err = ESP_OK;
        {
            Guard silence{};
            if constexpr (requires { PatchPolicy::store_instruction(detour.target, detour.instruction); }) {
                patch_err = PatchPolicy::store_instruction(detour.target, detour.instruction);
            } else {
                __atomic_store_n(static_cast<std::uint32_t*>(detour.target), detour.instruction, __ATOMIC_RELEASE);
            }
        }

        if (patch_err == ESP_OK) {
            if constexpr (requires { PatchPolicy::sync_code(detour.target, sizeof(std::uint32_t)); }) {
                patch_err = PatchPolicy::sync_code(detour.target, sizeof(std::uint32_t));
            }
        }
        auto unlock_err = ESP_OK;
        if constexpr (requires { PatchPolicy::unlock_all(); }) {
            unlock_err = PatchPolicy::unlock_all();
        }
        if (patch_err != ESP_OK) {
            return Status{fail(patch_err)};
        }
        if (unlock_err != ESP_OK) {
            return Status{fail(unlock_err)};
        }
        return ok();
    }
};

}  // namespace arc
