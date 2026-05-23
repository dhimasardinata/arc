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
    static_life,
};

struct Claim {
    Kind kind{};
    std::uint32_t subject{};
    std::uint32_t bound{};
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
using StaticLife = Fact<Kind::static_life, Subject>;

template <typename T>
concept ProofFact = requires {
    { T::claim() } -> std::same_as<Claim>;
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

}  // namespace arc::proof
