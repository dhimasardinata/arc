#pragma once

#include <cstddef>
#include <cstdint>

#define MALLOC_CAP_SPIRAM 0x01U
#define MALLOC_CAP_8BIT 0x02U
#define MALLOC_CAP_INTERNAL 0x04U

inline std::size_t heap_caps_get_free_size(std::uint32_t) noexcept
{
    return 0U;
}
