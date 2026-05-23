#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <utility>

#include "esp_err.h"

#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif

#if __has_include("mbedtls/bignum.h") && defined(SOC_MPI_SUPPORTED) && SOC_MPI_SUPPORTED
#include "arc/mpi.hpp"
#define ARC_CRYPTO_PAILLIER_HAS_MPI 1
#else
#define ARC_CRYPTO_PAILLIER_HAS_MPI 0
#endif

namespace arc::crypto {

template <typename Integer>
struct PaillierPublic {
    Integer n;
    Integer n2;
    Integer g;
};

template <typename Integer>
struct Paillier {
    using Public = PaillierPublic<Integer>;

    template <typename Result>
    [[nodiscard]] static Result fail(const typename Result::error_type err)
    {
        return Result{std::unexpected(err)};
    }

    [[nodiscard]] static auto encrypt(
        const Public& key,
        const Integer& message,
        const Integer& random)
    {
        auto gm = key.g.exp_mod(message, key.n2);
        using Result = decltype(gm);
        if (!gm) {
            return fail<Result>(gm.error());
        }

        auto rn = random.exp_mod(key.n, key.n2);
        if (!rn) {
            return fail<Result>(rn.error());
        }
        return gm->mul_mod(*rn, key.n2);
    }

    [[nodiscard]] static auto add(
        const Public& key,
        const Integer& lhs_cipher,
        const Integer& rhs_cipher)
    {
        return lhs_cipher.mul_mod(rhs_cipher, key.n2);
    }

    [[nodiscard]] static auto add_plain(
        const Public& key,
        const Integer& cipher,
        const Integer& message)
    {
        auto gm = key.g.exp_mod(message, key.n2);
        using Result = decltype(gm);
        if (!gm) {
            return fail<Result>(gm.error());
        }
        return cipher.mul_mod(*gm, key.n2);
    }

    [[nodiscard]] static auto rerandomize(
        const Public& key,
        const Integer& cipher,
        const Integer& random)
    {
        auto rn = random.exp_mod(key.n, key.n2);
        using Result = decltype(rn);
        if (!rn) {
            return fail<Result>(rn.error());
        }
        return cipher.mul_mod(*rn, key.n2);
    }

    [[nodiscard]] static auto aggregate(
        const Public& key,
        const std::span<const Integer> ciphertexts)
    {
        if (!ciphertexts.empty() && ciphertexts.data() == nullptr) {
            using Result = decltype(ciphertexts[0].clone());
            return fail<Result>(ESP_ERR_INVALID_ARG);
        }

        auto acc = ciphertexts.empty() ? key.g.exp_mod(Integer{0}, key.n2) : ciphertexts[0].clone();
        using Result = decltype(acc);
        if (!acc) {
            return fail<Result>(acc.error());
        }

        for (std::size_t i = ciphertexts.empty() ? 0U : 1U; i < ciphertexts.size(); ++i) {
            auto next = acc->mul_mod(ciphertexts[i], key.n2);
            if (!next) {
                return fail<Result>(next.error());
            }
            *acc = std::move(*next);
        }
        return acc;
    }
};

#if ARC_CRYPTO_PAILLIER_HAS_MPI
using PaillierMpi = Paillier<Mpi>;
using PaillierMpiPublic = PaillierPublic<Mpi>;
#endif

}  // namespace arc::crypto

#undef ARC_CRYPTO_PAILLIER_HAS_MPI
