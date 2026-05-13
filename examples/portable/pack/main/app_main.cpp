#include <array>
#include <cstdint>
#include <cstdio>

#include "arc/pack.hpp"
#include "arc/soc/target.hpp"

namespace {

struct Sample {
    std::uint16_t seq{};
    std::int16_t temp{};
    std::uint32_t flags{};
};

}  // namespace

ARC_PACK_REFLECT(Sample, &Sample::seq, &Sample::temp, &Sample::flags);

extern "C" void app_main()
{
    using Codec = arc::pack::Reflect<Sample>;

    const Sample in{
        .seq = 42U,
        .temp = 2731,
        .flags = 0xA5A5'0001U,
    };
    std::array<std::uint8_t, Codec::bytes> wire{};
    Sample out{};

    const auto encoded = arc::pack::serialize<arc::pack::Endian::big, Codec>(wire, in);
    if (encoded) {
        static_cast<void>(arc::pack::deserialize<arc::pack::Endian::big, Codec>(*encoded, out));
    }

    std::printf(
        "arc-portable target=%s arch=%s seq=%u temp=%d flags=0x%08lx\n",
        arc::soc::name,
        arc::soc::arch,
        static_cast<unsigned>(out.seq),
        static_cast<int>(out.temp),
        static_cast<unsigned long>(out.flags));
}
