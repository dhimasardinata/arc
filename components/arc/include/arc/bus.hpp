#pragma once

#include <cstddef>
#include <cstdint>

#include "arc/reg.hpp"
#include "arc/ring.hpp"

namespace arc {

template <typename Event, typename Control, std::size_t Capacity>
struct Bus {
    using event_type = Event;
    using control_type = Control;

    Ring<Event, Capacity> events{};
    Reg<std::uint32_t> pace{};
    Reg<Control> control{};
};

}  // namespace arc
