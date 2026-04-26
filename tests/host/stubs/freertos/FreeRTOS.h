#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

using BaseType_t = int;
using UBaseType_t = unsigned;
using StackType_t = std::uint32_t;
using TickType_t = std::uint32_t;

struct StaticTask_t {
    void* pad[4];
};

#define tskNO_AFFINITY (-1)
#define portBYTE_ALIGNMENT 16
#define configMAX_PRIORITIES 25

#define configASSERT(expr) \
    do {                   \
        if (!(expr)) {     \
            std::abort();  \
        }                  \
    } while (0)
