#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace arc::proof {

enum class Kind : std::uint8_t {
    deadline,
    no_heap,
    no_block,
    no_null,
    no_divide,
    static_life,
    misra_subset,
};

struct Claim {
    Kind kind{};
    std::uint32_t subject{};
    std::uint32_t bound{};
};

struct CertificateHeader {
    std::uint32_t magic{};
    std::uint32_t version{};
    std::uint32_t cycles{};
    std::uint32_t claims{};
};

template <Kind K, std::uint32_t Subject, std::uint32_t Bound = 0U>
struct Fact {
    static_assert(Subject != 0U,
                  "[ARC ERROR] arc::proof::Fact needs a subject. Action: use a stable non-zero module or workload id.");

    static constexpr Kind kind = K;
    static constexpr std::uint32_t subject = Subject;
    static constexpr std::uint32_t bound = Bound;

    [[nodiscard]] static consteval Claim claim() noexcept
    {
        return Claim{.kind = kind, .subject = subject, .bound = bound};
    }
};

template <std::uint32_t Subject, std::uint32_t Cycles>
using Deadline = Fact<Kind::deadline, Subject, Cycles>;

template <std::uint32_t Subject>
using NoHeap = Fact<Kind::no_heap, Subject>;

template <std::uint32_t Subject>
using NoBlock = Fact<Kind::no_block, Subject>;

template <std::uint32_t Subject>
using NoNull = Fact<Kind::no_null, Subject>;

template <std::uint32_t Subject>
using NoDivide = Fact<Kind::no_divide, Subject>;

template <std::uint32_t Subject>
using StaticLife = Fact<Kind::static_life, Subject>;

template <std::uint32_t Subject>
using MisraSubset = Fact<Kind::misra_subset, Subject>;

template <typename T>
concept ProofFact = requires {
    { T::claim() } -> std::same_as<Claim>;
};

template <typename T>
concept ProofPack = requires {
    { T::cycles } -> std::convertible_to<std::uint32_t>;
    { T::count } -> std::convertible_to<std::size_t>;
    T::claims();
};

template <std::uint32_t Cycles, typename... Facts>
struct Pack {
    static_assert(Cycles > 0U,
                  "[ARC ERROR] arc::proof::Pack needs a cycle budget. Action: set Cycles to the verified bound.");
    static_assert(sizeof...(Facts) > 0U,
                  "[ARC ERROR] arc::proof::Pack needs at least one fact. Action: add proof::Fact entries.");
    static_assert((ProofFact<Facts> && ...),
                  "[ARC ERROR] arc::proof::Pack facts must expose claim(). Action: use proof::Fact.");

    static constexpr std::uint32_t cycles = Cycles;
    static constexpr std::size_t count = sizeof...(Facts);

    [[nodiscard]] static consteval std::array<Claim, count> claims() noexcept
    {
        return {Facts::claim()...};
    }

    template <Kind Target>
    [[nodiscard]] static consteval bool has() noexcept
    {
        return ((Facts::kind == Target) || ...);
    }

    template <Kind Target, std::uint32_t Subject>
    [[nodiscard]] static consteval bool has() noexcept
    {
        static_assert(Subject != 0U,
                      "[ARC ERROR] arc::proof::Pack subject query needs a non-zero subject.");
        return (((Facts::kind == Target) && (Facts::subject == Subject)) || ...);
    }

    template <Kind Target>
    [[nodiscard]] static consteval std::uint32_t bound() noexcept
    {
        std::uint32_t out{};
        ((Facts::kind == Target ? out = Facts::bound : out), ...);
        return out;
    }

    template <Kind Target, std::uint32_t Subject>
    [[nodiscard]] static consteval std::uint32_t bound() noexcept
    {
        static_assert(Subject != 0U,
                      "[ARC ERROR] arc::proof::Pack subject query needs a non-zero subject.");
        std::uint32_t out{};
        (((Facts::kind == Target && Facts::subject == Subject) ? out = Facts::bound : out), ...);
        return out;
    }
};

template <ProofPack Pack, std::uint32_t Version = 1U>
struct Certificate {
    static_assert(Version != 0U,
                  "[ARC ERROR] arc::proof::Certificate needs a version. Action: use a non-zero evidence version.");

    static constexpr std::uint32_t magic = 0x41524350U;  // ARCP
    static constexpr std::uint32_t version = Version;
    static constexpr std::uint32_t cycles = Pack::cycles;
    static constexpr std::size_t count = Pack::count;

    [[nodiscard]] static consteval CertificateHeader header() noexcept
    {
        return {
            .magic = magic,
            .version = version,
            .cycles = cycles,
            .claims = static_cast<std::uint32_t>(count),
        };
    }

    [[nodiscard]] static consteval auto claims() noexcept
    {
        return Pack::claims();
    }

    template <Kind... Required>
    [[nodiscard]] static consteval bool covers() noexcept
    {
        return (true && ... && Pack::template has<Required>());
    }
};

template <typename T>
concept ProofCertificate = requires {
    { T::cycles } -> std::convertible_to<std::uint32_t>;
    { T::count } -> std::convertible_to<std::size_t>;
    { T::header() } -> std::same_as<CertificateHeader>;
    T::claims();
};

namespace detail {

[[nodiscard]] consteval std::uint32_t proof_max(const std::uint32_t first) noexcept
{
    return first;
}

template <typename... Rest>
[[nodiscard]] consteval std::uint32_t proof_max(const std::uint32_t first, const Rest... rest) noexcept
{
    const auto tail = proof_max(rest...);
    return first > tail ? first : tail;
}

}  // namespace detail

template <ProofCertificate... Certificates>
struct SafetyCase {
    static_assert(sizeof...(Certificates) > 0U,
                  "[ARC ERROR] arc::proof::SafetyCase needs at least one certificate. Action: add proof::Certificate entries.");

    static constexpr std::size_t certificates = sizeof...(Certificates);
    static constexpr std::size_t total_claims = (std::size_t{} + ... + Certificates::count);
    static constexpr std::uint32_t max_cycles = detail::proof_max(Certificates::cycles...);

    [[nodiscard]] static consteval std::array<CertificateHeader, certificates> headers() noexcept
    {
        return {Certificates::header()...};
    }

    template <Kind... Required>
    [[nodiscard]] static consteval bool covers() noexcept
    {
        return (true && ... && has<Required>());
    }

private:
    template <Kind Target>
    [[nodiscard]] static consteval bool has() noexcept
    {
        return (Certificates::template covers<Target>() || ...);
    }
};

}  // namespace arc::proof
