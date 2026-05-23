#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "esp_err.h"

#include "arc/pms.hpp"
#include "arc/result.hpp"

namespace arc {

namespace detail {

template <typename T, std::size_t Extent>
[[nodiscard]] constexpr bool tee_valid_span(const std::span<T, Extent> data) noexcept
{
    return data.empty() || data.data() != nullptr;
}

}  // namespace detail

enum class World : std::uint32_t {
    trusted = 0U,
    untrusted = 1U,
};

struct WorldRegion {
    const void* base{};
    std::size_t bytes{};
    World owner{World::trusted};
    PmsAccess trusted{PmsAccess::read_write};
    PmsAccess untrusted{PmsAccess::none};
};

enum class TeeAsset : std::uint8_t {
    code,
    data,
    crypto,
    efuse,
    peripheral,
};

template <TeeAsset Asset, typename T>
struct TrustedObject {
    static_assert(std::is_trivially_copyable_v<T>, "trusted object must be trivially copyable");

    alignas(T) T value{};

    [[nodiscard]] constexpr T& get() noexcept
    {
        return value;
    }

    [[nodiscard]] constexpr const T& get() const noexcept
    {
        return value;
    }
};

struct TeePlan {
    std::span<const WorldRegion> regions{};
    std::span<const std::uint32_t> trusted_peripherals{};
    std::span<const std::uint32_t> untrusted_peripherals{};
    std::uint32_t trusted_core{1U};
    std::uint32_t untrusted_core{0U};
};

template <typename Policy>
struct WorldGuard {
    [[nodiscard]] static esp_err_t configure(std::span<const WorldRegion> regions) noexcept
    {
        return Policy::configure(regions);
    }

    [[nodiscard]] static esp_err_t core_world(
        const std::uint32_t core,
        const World world) noexcept
    {
        return Policy::core_world(core, world);
    }

    [[nodiscard]] static esp_err_t peripheral_world(
        const std::uint32_t peripheral,
        const World world) noexcept
    {
        return Policy::peripheral_world(peripheral, world);
    }

    [[nodiscard]] static Status apply(const TeePlan& plan) noexcept
    {
        if (!detail::tee_valid_span(plan.regions) ||
            !detail::tee_valid_span(plan.trusted_peripherals) ||
            !detail::tee_valid_span(plan.untrusted_peripherals)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }
        if (const auto err = configure(plan.regions); err != ESP_OK) {
            return Status{fail(err)};
        }
        if (const auto err = core_world(plan.trusted_core, World::trusted); err != ESP_OK) {
            return Status{fail(err)};
        }
        if (const auto err = core_world(plan.untrusted_core, World::untrusted); err != ESP_OK) {
            return Status{fail(err)};
        }

        for (const auto peripheral : plan.trusted_peripherals) {
            if (const auto err = peripheral_world(peripheral, World::trusted); err != ESP_OK) {
                return Status{fail(err)};
            }
        }
        for (const auto peripheral : plan.untrusted_peripherals) {
            if (const auto err = peripheral_world(peripheral, World::untrusted); err != ESP_OK) {
                return Status{fail(err)};
            }
        }
        return ok();
    }
};

}  // namespace arc
