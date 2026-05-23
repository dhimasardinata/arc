#pragma once

#include <cstdint>

#include "arc/soc/esp32s3.hpp"
#include "arc/soc/esp32s31.hpp"
#include "arc/soc/esp32p4.hpp"

#if defined(CONFIG_IDF_TARGET_ESP32S31) && !defined(ARC_TARGET_ESP32S31)
#define ARC_TARGET_ESP32S31 1
#endif

#if defined(CONFIG_IDF_TARGET_ESP32P4) && !defined(ARC_TARGET_ESP32P4)
#define ARC_TARGET_ESP32P4 1
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(ARC_TARGET_ESP32S3)
#define ARC_TARGET_ESP32S3 1
#endif

#if (defined(ARC_TARGET_ESP32S31) && defined(ARC_TARGET_ESP32P4)) || \
    (defined(ARC_TARGET_ESP32S31) && defined(ARC_TARGET_ESP32S3)) || \
    (defined(ARC_TARGET_ESP32P4) && defined(ARC_TARGET_ESP32S3))
#error "Arc targets are mutually exclusive"
#endif

#if defined(ARC_TARGET_ESP32S31)
#define ARC_TARGET_IS_ESP32S31 1
#define ARC_TARGET_IS_ESP32P4 0
#define ARC_TARGET_IS_ESP32S3 0
#define ARC_TARGET_ARCH_RISCV 1
#define ARC_TARGET_ARCH_XTENSA 0
#elif defined(ARC_TARGET_ESP32P4)
#define ARC_TARGET_IS_ESP32S31 0
#define ARC_TARGET_IS_ESP32P4 1
#define ARC_TARGET_IS_ESP32S3 0
#define ARC_TARGET_ARCH_RISCV 1
#define ARC_TARGET_ARCH_XTENSA 0
#else
#define ARC_TARGET_IS_ESP32S31 0
#define ARC_TARGET_IS_ESP32P4 0
#define ARC_TARGET_IS_ESP32S3 1
#define ARC_TARGET_ARCH_RISCV 0
#define ARC_TARGET_ARCH_XTENSA 1
#endif

namespace arc::soc {

enum class Cap : std::uint8_t {
    mask,
    trax,
    drive,
    sense,
    app,
    tight,
    ptp,
    ml,
    cache,
    dma,
    tee,
    world,
    amp,
    cam,
    control,
};

#if ARC_TARGET_IS_ESP32S31
using Target = Esp32S31;
#elif ARC_TARGET_IS_ESP32P4
using Target = Esp32P4;
#else
using Target = Esp32S3;
#endif

template <typename T>
inline constexpr bool is = false;

template <>
inline constexpr bool is<Target> = true;

inline constexpr bool s3 = is<Esp32S3>;
inline constexpr bool s31 = is<Esp32S31>;
inline constexpr bool p4 = is<Esp32P4>;
inline constexpr const char* name = Target::name;
inline constexpr const char* arch = Target::arch;

namespace detail {

template <Cap C, typename T>
[[nodiscard]] consteval bool ok() noexcept
{
    if constexpr (C == Cap::mask) {
        return T::mask;
    } else if constexpr (C == Cap::trax) {
        return T::trax;
    } else if constexpr (C == Cap::drive) {
        return T::Api::drive;
    } else if constexpr (C == Cap::sense) {
        return T::Api::sense;
    } else if constexpr (C == Cap::app) {
        return T::Api::app;
    } else if constexpr (C == Cap::tight) {
        return T::Api::tight;
    } else if constexpr (C == Cap::ptp) {
        return T::Api::ptp;
    } else if constexpr (C == Cap::ml) {
        return T::Api::ml;
    } else if constexpr (C == Cap::cache) {
        return T::Api::cache;
    } else if constexpr (C == Cap::dma) {
        return T::Api::dma;
    } else if constexpr (C == Cap::tee) {
        return T::Api::tee;
    } else if constexpr (C == Cap::world) {
        return T::Api::world;
    } else if constexpr (C == Cap::amp) {
        return T::Api::amp;
    } else if constexpr (C == Cap::cam) {
        return T::Api::cam;
    } else {
        return T::Api::control;
    }
}

}  // namespace detail

template <Cap cap, typename T = Target>
inline constexpr bool has = detail::ok<cap, T>();

template <auto>
inline constexpr bool never_v = false;

}  // namespace arc::soc
