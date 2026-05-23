#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <utility>

#include "esp_err.h"
#include "mbedtls/bignum.h"
#include "soc/soc_caps.h"

namespace arc {

static_assert(SOC_MPI_SUPPORTED, "MPI/RSA accelerator is not supported on this target");

template <typename T>
using MpiResult = std::expected<T, int>;

[[nodiscard]] constexpr std::unexpected<int> mpi_fail(const int err) noexcept
{
    return std::unexpected(err);
}

struct MpiHardware {
    MpiHardware()
    {
        esp_mpi_acquire_hardware();
    }

    MpiHardware(const MpiHardware&) = delete;
    MpiHardware& operator=(const MpiHardware&) = delete;

    MpiHardware(MpiHardware&& other) noexcept
        : locked_(std::exchange(other.locked_, false))
    {
    }

    MpiHardware& operator=(MpiHardware&& other) noexcept
    {
        if (this != &other) {
            release();
            locked_ = std::exchange(other.locked_, false);
        }
        return *this;
    }

    ~MpiHardware()
    {
        release();
    }

    void release() noexcept
    {
        if (locked_) {
            esp_mpi_release_hardware();
            locked_ = false;
        }
    }

    [[nodiscard]] explicit constexpr operator bool() const noexcept
    {
        return locked_;
    }

private:
    bool locked_{true};
};

class Mpi {
public:
    static constexpr std::uint32_t blocks = SOC_MPI_MEM_BLOCKS_NUM;
    static constexpr std::uint32_t operations = SOC_MPI_OPERATIONS_NUM;
    static constexpr std::size_t max_bits = SOC_RSA_MAX_BIT_LEN;
    static constexpr std::size_t max_bytes = max_bits / 8U;

    Mpi()
    {
        mbedtls_mpi_init(&value_);
    }

    explicit Mpi(const mbedtls_mpi_sint value)
        : Mpi()
    {
        static_cast<void>(set(value));
    }

    Mpi(const Mpi&) = delete;
    Mpi& operator=(const Mpi&) = delete;

    Mpi(Mpi&& other) noexcept
        : Mpi()
    {
        mbedtls_mpi_swap(&value_, &other.value_);
    }

    Mpi& operator=(Mpi&& other) noexcept
    {
        if (this != &other) {
            mbedtls_mpi_swap(&value_, &other.value_);
        }
        return *this;
    }

    ~Mpi()
    {
        mbedtls_mpi_free(&value_);
    }

    [[nodiscard]] static MpiHardware lock()
    {
        return MpiHardware{};
    }

    [[nodiscard]] MpiResult<Mpi> clone() const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_copy(&out.value_, &value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] static MpiResult<Mpi> from_be(const std::span<const std::uint8_t> bytes)
    {
        Mpi out;
        if (bytes.empty()) {
            if (const auto err = out.set(0); err != 0) {
                return mpi_fail(err);
            }
            return out;
        }
        if (!valid_span(bytes)) {
            return mpi_fail(ESP_ERR_INVALID_ARG);
        }

        if (const auto err = mbedtls_mpi_read_binary(&out.value_, bytes.data(), bytes.size_bytes()); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] static MpiResult<Mpi> from_le(const std::span<const std::uint8_t> bytes)
    {
        Mpi out;
        if (bytes.empty()) {
            if (const auto err = out.set(0); err != 0) {
                return mpi_fail(err);
            }
            return out;
        }
        if (!valid_span(bytes)) {
            return mpi_fail(ESP_ERR_INVALID_ARG);
        }

        if (const auto err = mbedtls_mpi_read_binary_le(&out.value_, bytes.data(), bytes.size_bytes()); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] int set(const mbedtls_mpi_sint value) noexcept
    {
        return mbedtls_mpi_lset(&value_, value);
    }

    [[nodiscard]] std::size_t bytes() const noexcept
    {
        return mbedtls_mpi_size(&value_);
    }

    [[nodiscard]] std::size_t bits() const noexcept
    {
        return mbedtls_mpi_bitlen(&value_);
    }

    [[nodiscard]] int compare(const Mpi& other) const noexcept
    {
        return mbedtls_mpi_cmp_mpi(&value_, &other.value_);
    }

    [[nodiscard]] int write_be(const std::span<std::uint8_t> out) const noexcept
    {
        if (!valid_span(out)) {
            return ESP_ERR_INVALID_ARG;
        }
        return mbedtls_mpi_write_binary(&value_, out.data(), out.size_bytes());
    }

    [[nodiscard]] int write_le(const std::span<std::uint8_t> out) const noexcept
    {
        if (!valid_span(out)) {
            return ESP_ERR_INVALID_ARG;
        }
        return mbedtls_mpi_write_binary_le(&value_, out.data(), out.size_bytes());
    }

    [[nodiscard]] MpiResult<Mpi> add(const Mpi& rhs) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_add_mpi(&out.value_, &value_, &rhs.value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> sub(const Mpi& rhs) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_sub_mpi(&out.value_, &value_, &rhs.value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> mul(const Mpi& rhs) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_mul_mpi(&out.value_, &value_, &rhs.value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> mod(const Mpi& modulus) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_mod_mpi(&out.value_, &value_, &modulus.value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> mul_mod(const Mpi& rhs, const Mpi& modulus) const
    {
        Mpi out;
#if defined(CONFIG_MBEDTLS_HARDWARE_MPI) && CONFIG_MBEDTLS_HARDWARE_MPI
        const auto err = esp_mpi_mul_mpi_mod(&out.value_, &value_, &rhs.value_, &modulus.value_);
#else
        const auto product = mul(rhs);
        if (!product) {
            return mpi_fail(product.error());
        }
        const auto err = mbedtls_mpi_mod_mpi(&out.value_, &product->value_, &modulus.value_);
#endif
        if (err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> exp_mod(const Mpi& exponent, const Mpi& modulus, Mpi* const rr = nullptr) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_exp_mod(
                &out.value_,
                &value_,
                &exponent.value_,
                &modulus.value_,
                rr == nullptr ? nullptr : &rr->value_);
            err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] MpiResult<Mpi> inv_mod(const Mpi& modulus) const
    {
        Mpi out;
        if (const auto err = mbedtls_mpi_inv_mod(&out.value_, &value_, &modulus.value_); err != 0) {
            return mpi_fail(err);
        }
        return out;
    }

    [[nodiscard]] mbedtls_mpi& native() noexcept
    {
        return value_;
    }

    [[nodiscard]] const mbedtls_mpi& native() const noexcept
    {
        return value_;
    }

private:
    template <typename T, std::size_t Extent>
    [[nodiscard]] static constexpr bool valid_span(const std::span<T, Extent> value) noexcept
    {
        return value.empty() || value.data() != nullptr;
    }

    mbedtls_mpi value_{};
};

}  // namespace arc
