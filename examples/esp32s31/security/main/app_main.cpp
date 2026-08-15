#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>

#include "arc/board/esp32s31_korvo.hpp"
#include "arc/cloak.hpp"
#include "arc/puf.hpp"
#include "arc/secure_boot.hpp"
#include "arc/soc/target.hpp"
#include "arc/tee.hpp"

namespace {

using Board = arc::board::Korvo1;

struct SecureBootPolicy {
    static arc::Result<arc::secure::BootState> boot_state() noexcept
    {
        return arc::ok(arc::secure::BootState{
            .enabled = arc::soc::Target::secure_boot,
            .digest_valid = true,
            .revoked_keys = 0U,
        });
    }

    static esp_err_t boot_revoke(const std::uint8_t key_index) noexcept
    {
        return key_index < 3U ? ESP_OK : ESP_ERR_INVALID_ARG;
    }

    static arc::Result<arc::secure::BootDigest> boot_digest() noexcept
    {
        arc::secure::BootDigest digest{};
        digest.sha256[0] = 0x31U;
        digest.sha256[31] = 0x5aU;
        return arc::ok(digest);
    }
};

struct WorldPolicy {
    static esp_err_t configure(const std::span<const arc::WorldRegion> regions) noexcept
    {
        return regions.empty() ? ESP_ERR_INVALID_ARG : ESP_OK;
    }

    static esp_err_t core_world(const std::uint32_t core, const arc::World) noexcept
    {
        return core < arc::soc::Target::cores ? ESP_OK : ESP_ERR_INVALID_ARG;
    }

    static esp_err_t peripheral_world(const std::uint32_t peripheral, const arc::World) noexcept
    {
        return peripheral == 0U ? ESP_ERR_INVALID_ARG : ESP_OK;
    }
};

struct CloakPolicy {
    static constexpr std::uint32_t rng() noexcept
    {
        return 0x31U;
    }
};

struct PufHash {
    static arc::Result<std::array<std::uint8_t, 32>> sha256(const std::span<const std::uint8_t> stable) noexcept
    {
        if (stable.empty() || stable.data() == nullptr) {
            return arc::fail(ESP_ERR_INVALID_ARG);
        }
        std::array<std::uint8_t, 32> out{};
        out[0] = static_cast<std::uint8_t>(stable.size());
        out[31] = stable.back();
        return arc::ok(out);
    }
};

inline constexpr auto trusted_regions = std::array{
    arc::WorldRegion{
        .base = nullptr,
        .bytes = 4096U,
        .owner = arc::World::trusted,
        .trusted = arc::PmsAccess::read_write,
        .untrusted = arc::PmsAccess::none,
    },
};
inline constexpr auto trusted_peripherals = std::array<std::uint32_t, 2>{1U, 2U};
inline constexpr auto untrusted_peripherals = std::array<std::uint32_t, 1>{3U};
inline constexpr auto tee_plan = arc::TeePlan{
    .regions = trusted_regions,
    .trusted_peripherals = trusted_peripherals,
    .untrusted_peripherals = untrusted_peripherals,
    .trusted_core = 1U,
    .untrusted_core = 0U,
};

static_assert(arc::soc::Target::secure_boot);
static_assert(arc::soc::Target::flash_encryption);
static_assert(arc::soc::Target::tee);
static_assert(arc::soc::Target::puf);
static_assert(arc::soc::Target::worldguard);
static_assert(arc::soc::has<arc::soc::Cap::tee>);
static_assert(arc::soc::has<arc::soc::Cap::world>);
static_assert(Board::Module::flash_mb == 16U && Board::Module::psram_mb == 16U);
static_assert(tee_plan.trusted_core == 1U && tee_plan.untrusted_core == 0U);
static_assert(tee_plan.trusted_peripherals.size() == 2U);

}  // namespace

extern "C" void app_main()
{
    static_assert(arc::soc::s31, "security requires ARC_TARGET=esp32s31");
    static_assert(arc::Topology<Board>);

    const auto boot = arc::secure::SecureBoot::state<SecureBootPolicy>();
    const auto digest = arc::secure::SecureBoot::digest<SecureBootPolicy>();
    const auto revoke = arc::secure::SecureBoot::revoke<SecureBootPolicy>(0U);
    const auto world = arc::WorldGuard<WorldPolicy>::apply(tee_plan);

    const auto raw = std::array<std::uint8_t, 1>{0b0001'1011U};
    std::array<std::uint8_t, 1> stable{};
    const auto puf_stats = arc::crypto::Puf::von_neumann(raw, stable);
    const auto puf_key = arc::crypto::Puf::derive_with<PufHash>(stable);

    const auto dummy = std::array<std::byte, 4>{
        std::byte{0x31},
        std::byte{0x5a},
        std::byte{0xa5},
        std::byte{0xc3},
    };
    const auto cloak = arc::crypto::Cloak::scramble<CloakPolicy>(
        {.stall_mask = 0x03U, .dummy_reads = 2U},
        std::span<const std::byte>{dummy});

    const auto ready = boot && digest && revoke && world && puf_key && (*boot).enabled && puf_stats.stable_bits == 2U;
    std::printf(
        "arc-s31-security target=%s board=%s arch=%s secure_boot=%d flash_enc=%d tee=%d puf=%d worldguard=%d boot_enabled=%d digest0=%u world=%d puf_bits=%zu puf_key0=%u cloak_stalls=%u cloak_reads=%u ready=%d\n",
        arc::soc::name,
        Board::name,
        arc::soc::arch,
        arc::soc::Target::secure_boot ? 1 : 0,
        arc::soc::Target::flash_encryption ? 1 : 0,
        arc::soc::Target::tee ? 1 : 0,
        arc::soc::Target::puf ? 1 : 0,
        arc::soc::Target::worldguard ? 1 : 0,
        boot && (*boot).enabled ? 1 : 0,
        digest ? static_cast<unsigned>((*digest).sha256[0]) : 0U,
        arc::status_code(world),
        puf_stats.stable_bits,
        puf_key ? static_cast<unsigned>((*puf_key)[0]) : 0U,
        cloak.stalls,
        cloak.dummy_reads,
        ready ? 1 : 0);
}
