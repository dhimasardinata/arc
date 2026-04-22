#pragma once

#include <cstddef>

#include "arc/spsc.hpp"

namespace arc {

template <typename T, std::size_t Capacity>
using Ring = Spsc<T, Capacity>;

}  // namespace arc
