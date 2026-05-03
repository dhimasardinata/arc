#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory_resource>
#include <new>

#include "esp_heap_caps.h"

namespace arc {

template <std::uint32_t Caps>
struct PmrCapsResource final : std::pmr::memory_resource {
private:
    [[nodiscard]] void* do_allocate(
        const std::size_t bytes,
        const std::size_t alignment) override
    {
        void* const out = heap_caps_aligned_alloc(alignment, bytes, Caps);
        if (out == nullptr) {
#if defined(__cpp_exceptions)
            throw std::bad_alloc{};
#else
            std::abort();
#endif
        }
        return out;
    }

    void do_deallocate(
        void* const ptr,
        std::size_t,
        std::size_t) override
    {
        heap_caps_free(ptr);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

}  // namespace arc
