#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_mac.h"
#include "soc/soc_caps.h"

#include "arc/result.hpp"

namespace arc {

struct Fuse {
    static_assert(SOC_EFUSE_SUPPORTED, "eFuse is not supported on this target");

    using Mac = std::array<std::uint8_t, 6>;

    [[nodiscard]] static int bits(const esp_efuse_desc_t* field[]) noexcept
    {
        return esp_efuse_get_field_size(field);
    }

    [[nodiscard]] static bool bit(const esp_efuse_desc_t* field[]) noexcept
    {
        return esp_efuse_read_field_bit(field);
    }

    [[nodiscard]] static esp_err_t read(
        const esp_efuse_desc_t* field[],
        void* const data,
        const std::size_t size_bits) noexcept
    {
        return esp_efuse_read_field_blob(field, data, size_bits);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] static Result<T> read(const esp_efuse_desc_t* field[]) noexcept
    {
        T out{};
        const auto err = read(field, &out, sizeof(T) * 8U);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static Result<std::size_t> count(const esp_efuse_desc_t* field[]) noexcept
    {
        std::size_t out{};
        const auto err = esp_efuse_read_field_cnt(field, &out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static std::uint32_t word(
        const esp_efuse_block_t block,
        const unsigned reg) noexcept
    {
        return esp_efuse_read_reg(block, reg);
    }

    [[nodiscard]] static esp_err_t block(
        const esp_efuse_block_t block,
        void* const data,
        const std::size_t offset_bits,
        const std::size_t size_bits) noexcept
    {
        return esp_efuse_read_block(block, data, offset_bits, size_bits);
    }

    [[nodiscard]] static esp_err_t mac(
        Mac& out,
        const esp_mac_type_t type = ESP_MAC_BASE) noexcept
    {
        return esp_read_mac(out.data(), type);
    }

    [[nodiscard]] static Result<Mac> mac(const esp_mac_type_t type = ESP_MAC_BASE) noexcept
    {
        Mac out{};
        const auto err = mac(out, type);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t factory(Mac& out) noexcept
    {
        return esp_efuse_mac_get_default(out.data());
    }

    [[nodiscard]] static Result<Mac> factory() noexcept
    {
        Mac out{};
        const auto err = factory(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static esp_err_t custom(Mac& out) noexcept
    {
        return esp_efuse_mac_get_custom(out.data());
    }

    [[nodiscard]] static Result<Mac> custom() noexcept
    {
        Mac out{};
        const auto err = custom(out);
        if (err != ESP_OK) {
            return fail(err);
        }
        return out;
    }

    [[nodiscard]] static std::uint32_t pkg() noexcept
    {
        return esp_efuse_get_pkg_ver();
    }

    [[nodiscard]] static std::uint32_t secure() noexcept
    {
        return esp_efuse_read_secure_version();
    }

    [[nodiscard]] static bool empty(const esp_efuse_block_t block) noexcept
    {
        return esp_efuse_block_is_empty(block);
    }

    [[nodiscard]] static esp_efuse_coding_scheme_t code(const esp_efuse_block_t block) noexcept
    {
        return esp_efuse_get_coding_scheme(block);
    }

    [[nodiscard]] static esp_err_t check() noexcept
    {
        return esp_efuse_check_errors();
    }
};

}  // namespace arc
