#pragma once

namespace arc::arch {

struct Riscv {
    static constexpr const char* name = "riscv";
    static constexpr bool levels = false;
    static constexpr bool trax = false;
    static constexpr bool pin = true;
    static constexpr bool csr = true;
    static constexpr bool vec = true;
};

}  // namespace arc::arch
