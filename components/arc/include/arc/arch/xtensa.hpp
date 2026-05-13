#pragma once

namespace arc::arch {

struct Xtensa {
    static constexpr const char* name = "xtensa";
    static constexpr bool levels = true;
    static constexpr bool trax = true;
    static constexpr bool pin = true;
    static constexpr bool csr = false;
    static constexpr bool vec = false;
};

}  // namespace arc::arch
