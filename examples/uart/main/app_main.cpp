#include <array>
#include <cstdint>
#include <cstdio>

#include "arc.hpp"

namespace app {

using Serial = arc::Uart<UART_NUM_1, 17, 18>;

inline void boot()
{
    const auto init = Serial::init();
    std::printf("arc-uart init=0x%x tx=%d rx=%d\n", static_cast<unsigned>(init), Serial::tx(), Serial::rx());

    constexpr char msg[] = "arc-uart\n";
    const auto wrote = Serial::write(msg, sizeof(msg) - 1U);
    std::printf("arc-uart wrote=%u\n", wrote ? static_cast<unsigned>(*wrote) : 0U);

    std::size_t waiting{};
    static_cast<void>(Serial::available(waiting));
    std::array<std::uint8_t, 32> rx{};
    const auto got = Serial::read(rx.data(), rx.size(), 5U);
    std::printf("arc-uart waiting=%u got=%u\n", static_cast<unsigned>(waiting), got ? static_cast<unsigned>(*got) : 0U);
}

}  // namespace app

extern "C" void app_main()
{
    app::boot();
}
