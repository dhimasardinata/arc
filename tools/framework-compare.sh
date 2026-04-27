#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT

CXX="${CXX:-g++}"

cd "$ROOT"

IDF_ROOT="${ARC_IDF_PATH:-}"
if [[ -z "$IDF_ROOT" && -d esp-idf/.git ]]; then
    IDF_ROOT="$ROOT/esp-idf"
fi
if [[ -z "$IDF_ROOT" && -d "$HOME/esp-idf/.git" ]]; then
    IDF_ROOT="$HOME/esp-idf"
fi
if [[ -z "$IDF_ROOT" || ! -f "$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/include/mbedtls/base64.h" ]]; then
    echo "ESP-IDF source with mbedTLS is required for framework host benchmarks; set ARC_IDF_PATH or provide esp-idf/" >&2
    exit 1
fi

if [[ ! -d arduino-esp32/.git ]]; then
    "$ROOT/tools/ensure-frameworks.sh"
fi

echo "== framework source versions =="
printf 'ESP-IDF target  %s\n' "${ARC_IDF_TARGET:-esp32s3}"
if [[ -d "$IDF_ROOT/.git" ]]; then
    printf 'raw ESP-IDF     %s %s\n' \
        "$(git -C "$IDF_ROOT" rev-parse --short HEAD)" \
        "$(git -C "$IDF_ROOT" describe --tags --always --dirty 2>/dev/null || true)"
else
    printf 'raw ESP-IDF     %s\n' "$IDF_ROOT"
fi
printf 'Arduino-ESP32   %s %s\n' \
    "$(git -C arduino-esp32 rev-parse --short HEAD)" \
    "$(git -C arduino-esp32 describe --tags --always --dirty 2>/dev/null || true)"
if [[ -f arduino-esp32/platform.txt ]]; then
    awk -F= '/^version=/{print "Arduino core    " $2}' arduino-esp32/platform.txt
fi
if [[ -f arduino-esp32/idf_component.yml ]]; then
    awk -F\" '/idf:/{print "Arduino IDF     " $2}' arduino-esp32/idf_component.yml
fi

echo
echo "== framework host benchmark scope =="
echo "Measured here: host-compilable Arc, Arduino-ESP32 core, and ESP-IDF mbedTLS code paths."
echo "ESP32-S3 driver timing still belongs on-device; host CI only publishes executable host measurements."

echo
echo "== Arc host benchmark =="
if [[ "${ARC_SKIP_HOST_BENCH:-0}" == "1" ]]; then
    echo "skipped; caller already ran tools/host-bench.sh"
else
    "$ROOT/tools/host-bench.sh"
fi

mkdir -p "$BUILD/arduino"
cp arduino-esp32/cores/esp32/Print.cpp "$BUILD/arduino/Print.cpp"
cp arduino-esp32/cores/esp32/Print.h "$BUILD/arduino/Print.h"
cp arduino-esp32/cores/esp32/Printable.h "$BUILD/arduino/Printable.h"

cat >"$BUILD/arduino/Arduino.h" <<'STUB'
#pragma once
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
class __FlashStringHelper;
STUB

cat >"$BUILD/arduino/WString.h" <<'STUB'
#pragma once
#include <cstddef>
class String {
public:
    String() = default;
    explicit String(const char* value) : value_(value == nullptr ? "" : value) {}
    const char* c_str() const { return value_; }
    std::size_t length() const { std::size_t n = 0; while (value_[n] != '\0') { ++n; } return n; }
private:
    const char* value_{""};
};
STUB

cat >"$BUILD/framework-print-compare.cpp" <<'CPP'
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>

#include "Arduino.h"
#include "Print.h"
#include "arc/stream.hpp"
#include "arc/ws.hpp"

extern "C" {
#include "mbedtls/base64.h"
}

namespace {

using Clock = std::chrono::steady_clock;
volatile std::uint64_t sink{};
volatile std::uint32_t tick{0x42U};

std::uint8_t next_byte() noexcept
{
    tick = (tick * 1'103'515'245U) + 12'345U;
    return static_cast<std::uint8_t>(tick >> 16U);
}

void barrier() noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" ::: "memory");
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

void expect(const bool condition, const char* const message)
{
    if (!condition) {
        std::fprintf(stderr, "framework compare failed: %s\n", message);
        std::exit(1);
    }
}

template <typename Fn>
void bench(const char* const name, const std::uint64_t ops, Fn&& fn)
{
    const auto start = Clock::now();
    fn();
    const auto stop = Clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    std::printf("%-32s %12llu ops %10.2f ns/op\n",
                name,
                static_cast<unsigned long long>(ops),
                static_cast<double>(ns) / static_cast<double>(ops));
}

struct ArcSink {
    std::array<std::uint8_t, 8192> data{};
    std::size_t pos{};
    std::uint64_t checksum{};

    arc::Status send_all(const void* const src, const std::size_t bytes) noexcept
    {
        const auto* in = static_cast<const std::uint8_t*>(src);
        for (std::size_t i = 0; i < bytes; ++i) {
            const auto byte = in[i];
            data[(pos + i) % data.size()] = byte;
            checksum += byte;
        }
        pos += bytes;
        return arc::ok();
    }

    arc::Result<std::size_t> recv(void*, std::size_t) noexcept
    {
        return std::size_t{};
    }
};

class ArduinoBytePrint final : public Print {
public:
    std::array<std::uint8_t, 8192> data{};
    std::size_t pos{};
    std::uint64_t checksum{};

    std::size_t write(std::uint8_t value) override
    {
        data[pos % data.size()] = value;
        checksum += value;
        ++pos;
        return 1U;
    }
};

class ArduinoBulkPrint final : public Print {
public:
    std::array<std::uint8_t, 8192> data{};
    std::size_t pos{};
    std::uint64_t checksum{};

    std::size_t write(std::uint8_t value) override
    {
        data[pos % data.size()] = value;
        checksum += value;
        ++pos;
        return 1U;
    }

    std::size_t write(const std::uint8_t* buffer, std::size_t size) override
    {
        for (std::size_t i = 0; i < size; ++i) {
            const auto byte = buffer[i];
            data[(pos + i) % data.size()] = byte;
            checksum += byte;
        }
        pos += size;
        return size;
    }
};

template <std::size_t Bytes>
void bench_write_set(ArcSink& arc_sink, Print& byte_base, ArduinoBytePrint& byte_print, Print& bulk_base, ArduinoBulkPrint& bulk_print)
{
    constexpr std::uint32_t rounds = 300'000U;
    std::array<std::uint8_t, Bytes> payload{};
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i * 3U);
    }

    char arc_name[40]{};
    char byte_name[40]{};
    char bulk_name[40]{};
    std::snprintf(arc_name, sizeof(arc_name), "Arc write %zuB", Bytes);
    std::snprintf(byte_name, sizeof(byte_name), "Arduino byte write %zuB", Bytes);
    std::snprintf(bulk_name, sizeof(bulk_name), "Arduino bulk write %zuB", Bytes);

    bench(arc_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            barrier();
            expect(arc::net::Stream::write(arc_sink, std::span(payload)).has_value(), "Arc stream write");
        }
    });
    sink += arc_sink.pos + arc_sink.checksum + arc_sink.data[arc_sink.pos % arc_sink.data.size()];

    bench(byte_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            barrier();
            expect(byte_base.write(payload.data(), payload.size()) == payload.size(), "Arduino byte write");
        }
    });
    sink += byte_print.pos + byte_print.checksum + byte_print.data[byte_print.pos % byte_print.data.size()];

    bench(bulk_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            barrier();
            expect(bulk_base.write(payload.data(), payload.size()) == payload.size(), "Arduino bulk write");
        }
    });
    sink += bulk_print.pos + bulk_print.checksum + bulk_print.data[bulk_print.pos % bulk_print.data.size()];
}

template <std::size_t Bytes>
void bench_frame_set(ArcSink& arc_sink, Print& byte_base, Print& bulk_base)
{
    constexpr std::uint32_t rounds = 300'000U;
    std::array<std::uint8_t, Bytes> payload{};
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i * 5U);
    }

    char arc_name[40]{};
    char byte_name[40]{};
    char bulk_name[40]{};
    std::snprintf(arc_name, sizeof(arc_name), "Arc frame16 %zuB", Bytes);
    std::snprintf(byte_name, sizeof(byte_name), "Arduino byte frame16 %zuB", Bytes);
    std::snprintf(bulk_name, sizeof(bulk_name), "Arduino bulk frame16 %zuB", Bytes);

    bench(arc_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            barrier();
            expect(arc::net::Stream::write_frame16(arc_sink, std::span(payload)).has_value(), "Arc frame16");
        }
    });
    sink += arc_sink.pos + arc_sink.checksum;

    bench(byte_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            const std::array<std::uint8_t, 2> header{
                static_cast<std::uint8_t>(Bytes >> 8U),
                static_cast<std::uint8_t>(Bytes),
            };
            barrier();
            expect(byte_base.write(header.data(), header.size()) == header.size(), "Arduino byte frame header");
            expect(byte_base.write(payload.data(), payload.size()) == payload.size(), "Arduino byte frame payload");
        }
    });

    bench(bulk_name, rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[i % payload.size()] = next_byte();
            const std::array<std::uint8_t, 2> header{
                static_cast<std::uint8_t>(Bytes >> 8U),
                static_cast<std::uint8_t>(Bytes),
            };
            barrier();
            expect(bulk_base.write(header.data(), header.size()) == header.size(), "Arduino bulk frame header");
            expect(bulk_base.write(payload.data(), payload.size()) == payload.size(), "Arduino bulk frame payload");
        }
    });
}

}  // namespace

int main()
{
    std::puts("== real framework host compare ==");

    ArcSink arc_sink;
    ArduinoBytePrint byte_print;
    Print& byte_base = byte_print;
    ArduinoBulkPrint bulk_print;
    Print& bulk_base = bulk_print;

    bench_write_set<8>(arc_sink, byte_base, byte_print, bulk_base, bulk_print);
    bench_write_set<32>(arc_sink, byte_base, byte_print, bulk_base, bulk_print);
    bench_write_set<128>(arc_sink, byte_base, byte_print, bulk_base, bulk_print);

    bench_frame_set<8>(arc_sink, byte_base, bulk_base);
    bench_frame_set<32>(arc_sink, byte_base, bulk_base);
    bench_frame_set<128>(arc_sink, byte_base, bulk_base);

    constexpr std::uint32_t rounds = 500'000U;
    bench("Arduino Print::print u32", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            const auto value = static_cast<unsigned long>((static_cast<std::uint32_t>(next_byte()) << 16U) | i);
            barrier();
            expect(bulk_base.print(value, DEC) != 0U, "Arduino decimal print");
        }
    });
    sink += byte_print.pos + byte_print.checksum + bulk_print.pos + bulk_print.checksum;

    std::array<char, 32> arc_key{};
    std::array<unsigned char, 32> idf_key{};
    std::array<std::uint8_t, 16> nonce{};
    for (std::size_t i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<std::uint8_t>(i * 7U);
    }
    bench("Arc WS base64 key", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            nonce[i % nonce.size()] = next_byte();
            barrier();
            const auto out = arc::net::Ws::key(arc_key, std::span<const std::uint8_t>{nonce});
            expect(out.has_value() && out->size() == 24U, "Arc WS key");
            sink += static_cast<unsigned char>((*out)[0]);
        }
    });

    bench("ESP-IDF mbedTLS base64", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            nonce[i % nonce.size()] = next_byte();
            barrier();
            std::size_t out_len{};
            const auto err = mbedtls_base64_encode(
                idf_key.data(),
                idf_key.size(),
                &out_len,
                nonce.data(),
                nonce.size());
            expect(err == 0 && out_len == 24U, "ESP-IDF mbedTLS base64");
            sink += idf_key[0];
        }
    });

    std::printf("framework host compare: OK (%llu)\n", static_cast<unsigned long long>(sink));
}
CPP

"$CXX" \
    -std=gnu++23 \
    -O3 \
    -DNDEBUG \
    -Wall \
    -Wextra \
    -Werror \
    -pthread \
    -I"$BUILD/arduino" \
    -I"$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/include" \
    -I"$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/core" \
    -I"$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/drivers/builtin/include" \
    -I"$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/drivers/builtin/src" \
    -I"$ROOT/tests/host/stubs" \
    -I"$ROOT/components/arc/include" \
    "$BUILD/framework-print-compare.cpp" \
    "$BUILD/arduino/Print.cpp" \
    "$IDF_ROOT/components/mbedtls/mbedtls/tf-psa-crypto/drivers/builtin/src/base64.c" \
    -o "$BUILD/framework-print-compare"

echo
OUTPUT="$("$BUILD/framework-print-compare")"
printf '%s\n' "$OUTPUT"

if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    {
        echo
        echo "## Framework Host Benchmarks"
        echo
        echo '```text'
        printf '%s\n' "$OUTPUT"
        echo '```'
    } >>"$GITHUB_STEP_SUMMARY"
fi
