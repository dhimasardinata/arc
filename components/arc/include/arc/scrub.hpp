#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "arc/result.hpp"

namespace arc {

struct ScrubRegion {
    const std::byte* data{};
    std::size_t bytes{};
    std::uint32_t seal{};
    std::uint32_t tag{};
};

struct ScrubFault {
    std::size_t index{};
    std::uint32_t tag{};
    std::uint32_t expected{};
    std::uint32_t actual{};
    std::size_t bytes{};
};

struct ScrubSample {
    std::size_t index{};
    std::uint32_t tag{};
    std::uint32_t crc{};
    std::size_t bytes{};
};

template <std::size_t Regions>
struct Scrub {
    static_assert(Regions > 0U,
                  "[ARC ERROR] arc::Scrub needs at least one region. Action: set Regions to the fixed memory map size.");

    std::array<ScrubRegion, Regions> regions{};
    std::size_t cursor{};
    ScrubFault last{};

    [[nodiscard]] static std::uint32_t crc32(const std::span<const std::byte> bytes) noexcept
    {
        if (!valid(bytes)) {
            return 0U;
        }

        auto crc = 0xFFFF'FFFFU;
        for (const auto value : bytes) {
            crc ^= static_cast<std::uint8_t>(value);
            for (auto bit = 0U; bit < 8U; ++bit) {
                const auto mask = 0U - (crc & 1U);
                crc = (crc >> 1U) ^ (0xEDB8'8320U & mask);
            }
        }
        return crc ^ 0xFFFF'FFFFU;
    }

    [[nodiscard]] Status watch(
        const std::size_t index,
        const std::span<const std::byte> bytes,
        const std::uint32_t tag = 0U) noexcept
    {
        if (index >= Regions || bytes.empty() || !valid(bytes)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        regions[index] = ScrubRegion{
            .data = bytes.data(),
            .bytes = bytes.size(),
            .seal = crc32(bytes),
            .tag = tag,
        };
        return ok();
    }

    [[nodiscard]] Status refresh(const std::size_t index) noexcept
    {
        if (index >= Regions || !ready(index)) {
            return Status{fail(ESP_ERR_INVALID_ARG)};
        }

        auto& region = regions[index];
        region.seal = crc32(span(region));
        return ok();
    }

    [[nodiscard]] bool ready(const std::size_t index) const noexcept
    {
        return index < Regions && regions[index].data != nullptr && regions[index].bytes != 0U;
    }

    template <typename Policy = void>
    [[nodiscard]] Result<ScrubSample> scan() noexcept
    {
        for (std::size_t seen = 0U; seen < Regions; ++seen) {
            const auto index = cursor;
            cursor = next(cursor);
            if (!ready(index)) {
                continue;
            }

            const auto& region = regions[index];
            const auto actual = crc32(span(region));
            if (actual != region.seal) {
                last = ScrubFault{
                    .index = index,
                    .tag = region.tag,
                    .expected = region.seal,
                    .actual = actual,
                    .bytes = region.bytes,
                };
                capture<Policy>(last);
                halt<Policy>(last);
                return fail(ESP_ERR_INVALID_STATE);
            }

            return ScrubSample{
                .index = index,
                .tag = region.tag,
                .crc = actual,
                .bytes = region.bytes,
            };
        }

        return fail(ESP_ERR_NOT_FOUND);
    }

    template <typename Policy = void>
    [[nodiscard]] Result<std::size_t> step(const std::size_t budget = Regions) noexcept
    {
        if (budget == 0U) {
            return fail(ESP_ERR_INVALID_ARG);
        }

        std::size_t scanned{};
        for (std::size_t i = 0U; i < budget; ++i) {
            const auto sample = scan<Policy>();
            if (!sample) {
                if (sample.error() == ESP_ERR_NOT_FOUND) {
                    return scanned;
                }
                return fail(sample.error());
            }
            ++scanned;
        }
        return scanned;
    }

private:
    [[nodiscard]] static constexpr std::size_t next(const std::size_t index) noexcept
    {
        return index + 1U == Regions ? 0U : index + 1U;
    }

    [[nodiscard]] static constexpr bool valid(const std::span<const std::byte> bytes) noexcept
    {
        return bytes.empty() || bytes.data() != nullptr;
    }

    [[nodiscard]] static std::span<const std::byte> span(const ScrubRegion& region) noexcept
    {
        return {region.data, region.bytes};
    }

    template <typename Policy>
    static void capture(const ScrubFault& fault) noexcept
    {
        if constexpr (requires { Policy::capture(fault); }) {
            Policy::capture(fault);
        }
    }

    template <typename Policy>
    static void halt(const ScrubFault& fault) noexcept
    {
        if constexpr (requires { Policy::halt(fault); }) {
            static_cast<void>(Policy::halt(fault));
        } else if constexpr (requires { Policy::halt(); }) {
            static_cast<void>(Policy::halt());
        }
    }
};

}  // namespace arc
