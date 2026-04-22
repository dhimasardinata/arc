#pragma once

#include <cstddef>
#include <cstdint>

#include "arc/reg.hpp"
#include "arc/spsc.hpp"

namespace arc {

template <typename Event, typename Control, std::size_t Capacity>
struct Bus {
    using event_type = Event;
    using control_type = Control;

    Spsc<Event, Capacity> events{};
    Reg<std::uint32_t> pace{};
    Reg<Control> control{};
};

template <typename Event, typename Control, std::size_t Capacity>
using Link = Bus<Event, Control, Capacity>;

}  // namespace arc
