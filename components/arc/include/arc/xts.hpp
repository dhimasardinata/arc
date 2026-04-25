#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <type_traits>

#include "esp_err.h"
#include "esp_flash.h"
#include "esp_partition.h"

#include "arc/soc.hpp"

namespace arc {

template <typename T>
concept XtsBytes = std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct Xts {
    static_assert(Soc::flash_xts, "XTS flash encryption is not supported on this target");

    static constexpr std::size_t block = 16U;
    static constexpr std::size_t max_block = Soc::flash_xts_block;

    [[nodiscard]] static constexpr bool aligned(
        const std::uint32_t address,
        const std::size_t bytes) noexcept
    {
        return ((address | bytes) & (block - 1U)) == 0U;
    }

    [[nodiscard]] static esp_err_t read(
        const std::uint32_t address,
        void* const out,
        const std::size_t bytes) noexcept
    {
        if (bytes == 0U) {
            return ESP_OK;
        }
        if (out == nullptr || bytes > std::numeric_limits<std::uint32_t>::max()) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_flash_read_encrypted(
            nullptr,
            address,
            out,
            static_cast<std::uint32_t>(bytes));
    }

    template <typename T, std::size_t Extent>
        requires(!std::is_const_v<T> && XtsBytes<T>)
    [[nodiscard]] static esp_err_t read(
        const std::uint32_t address,
        const std::span<T, Extent> out) noexcept
    {
        return read(address, out.data(), out.size_bytes());
    }

    [[nodiscard]] static esp_err_t write(
        const std::uint32_t address,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        if (bytes == 0U) {
            return ESP_OK;
        }
        if (data == nullptr || bytes > std::numeric_limits<std::uint32_t>::max() || !aligned(address, bytes)) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_flash_write_encrypted(
            nullptr,
            address,
            data,
            static_cast<std::uint32_t>(bytes));
    }

    template <typename T, std::size_t Extent>
        requires XtsBytes<T>
    [[nodiscard]] static esp_err_t write(
        const std::uint32_t address,
        const std::span<T, Extent> data) noexcept
    {
        return write(address, data.data(), data.size_bytes());
    }

    [[nodiscard]] static bool encrypted(const esp_partition_t& part) noexcept
    {
        return part.encrypted;
    }

    [[nodiscard]] static esp_err_t read(
        const esp_partition_t* const part,
        const std::size_t offset,
        void* const out,
        const std::size_t bytes) noexcept
    {
        if (part == nullptr || (out == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_partition_read(part, offset, out, bytes);
    }

    template <typename T, std::size_t Extent>
        requires(!std::is_const_v<T> && XtsBytes<T>)
    [[nodiscard]] static esp_err_t read(
        const esp_partition_t* const part,
        const std::size_t offset,
        const std::span<T, Extent> out) noexcept
    {
        return read(part, offset, out.data(), out.size_bytes());
    }

    [[nodiscard]] static esp_err_t write(
        const esp_partition_t* const part,
        const std::size_t offset,
        const void* const data,
        const std::size_t bytes) noexcept
    {
        if (part == nullptr || (data == nullptr && bytes != 0U)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (part->encrypted &&
            (offset > std::numeric_limits<std::uint32_t>::max() ||
             !aligned(static_cast<std::uint32_t>(offset), bytes))) {
            return ESP_ERR_INVALID_ARG;
        }

        return esp_partition_write(part, offset, data, bytes);
    }

    template <typename T, std::size_t Extent>
        requires XtsBytes<T>
    [[nodiscard]] static esp_err_t write(
        const esp_partition_t* const part,
        const std::size_t offset,
        const std::span<T, Extent> data) noexcept
    {
        return write(part, offset, data.data(), data.size_bytes());
    }
};

}  // namespace arc
