#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_random.h"
#include "soc/soc_caps.h"

namespace arc {

struct Rng {
    static_assert(SOC_RNG_SUPPORTED, "hardware RNG is not supported on this target");

    [[nodiscard]] static std::uint32_t word() noexcept
    {
        return esp_random();
    }

    static void fill(void* const data, const std::size_t bytes) noexcept
    {
        if (data != nullptr && bytes != 0U) {
            esp_fill_random(data, bytes);
        }
    }

    template <typename T>
        requires(!std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    [[nodiscard]] static T value() noexcept
    {
        T out{};
        fill(&out, sizeof(out));
        return out;
    }

    template <typename T, std::size_t Extent>
        requires(!std::is_const_v<T> && std::is_trivially_copyable_v<T>)
    static void fill(const std::span<T, Extent> data) noexcept
    {
        fill(data.data(), data.size_bytes());
    }
};

}  // namespace arc
