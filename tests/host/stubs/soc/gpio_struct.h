#pragma once

#include <cstdint>

struct gpio_reg32_t {
    std::uint32_t val{};
};

struct gpio_dev_t {
    std::uint32_t out{};
    std::uint32_t out_w1ts{};
    std::uint32_t out_w1tc{};
    std::uint32_t enable_w1ts{};
    std::uint32_t enable_w1tc{};
    gpio_reg32_t out1{};
    gpio_reg32_t out1_w1ts{};
    gpio_reg32_t out1_w1tc{};
    gpio_reg32_t enable1_w1ts{};
    gpio_reg32_t enable1_w1tc{};
};

inline gpio_dev_t GPIO{};
