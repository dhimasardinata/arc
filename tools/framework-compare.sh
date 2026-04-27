#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$(mktemp -d)"
trap 'rm -rf "$BUILD"' EXIT

CXX="${CXX:-g++}"

cd "$ROOT"

if [[ ! -d arduino-esp32/.git ]]; then
    "$ROOT/tools/ensure-frameworks.sh"
fi

echo "== framework source versions =="
if [[ -d esp-idf/.git ]]; then
    printf 'raw ESP-IDF     %s %s\n' \
        "$(git -C esp-idf rev-parse --short HEAD)" \
        "$(git -C esp-idf describe --tags --always --dirty 2>/dev/null || true)"
else
    echo "raw ESP-IDF     missing local esp-idf/"
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
echo "== Arc host benchmark =="
"$ROOT/tools/host-bench.sh"

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

namespace {

using Clock = std::chrono::steady_clock;
volatile std::uint64_t sink{};
volatile std::uint32_t tick{0x42U};

[[gnu::noinline]] std::uint8_t next_byte() noexcept
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
    std::printf("%-28s %12llu ops %10.2f ns/op\n",
                name,
                static_cast<unsigned long long>(ops),
                static_cast<double>(ns) / static_cast<double>(ops));
}

struct ArcSink {
    std::array<std::uint8_t, 4096> data{};
    std::size_t pos{};

    [[gnu::noinline]] arc::Status send_all(const void* const src, const std::size_t bytes) noexcept
    {
        const auto* in = static_cast<const std::uint8_t*>(src);
        for (std::size_t i = 0; i < bytes; ++i) {
            data[(pos + i) % data.size()] = in[i];
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
    std::array<std::uint8_t, 4096> data{};
    std::size_t pos{};

    [[gnu::noinline]] std::size_t write(std::uint8_t value) override
    {
        data[pos % data.size()] = value;
        ++pos;
        return 1U;
    }
};

class ArduinoBulkPrint final : public Print {
public:
    std::array<std::uint8_t, 4096> data{};
    std::size_t pos{};

    [[gnu::noinline]] std::size_t write(std::uint8_t value) override
    {
        data[pos % data.size()] = value;
        ++pos;
        return 1U;
    }

    [[gnu::noinline]] std::size_t write(const std::uint8_t* buffer, std::size_t size) override
    {
        for (std::size_t i = 0; i < size; ++i) {
            data[(pos + i) % data.size()] = buffer[i];
        }
        pos += size;
        return size;
    }
};

}  // namespace

int main()
{
    constexpr std::uint32_t rounds = 500'000U;
    std::array<std::uint8_t, 32> payload{
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
    };

    std::puts("== real Arduino Print.cpp host compare ==");

    ArcSink arc_sink;
    bench("Arc Stream::write", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = next_byte();
            barrier();
            expect(arc::net::Stream::write(arc_sink, std::span(payload)).has_value(), "Arc stream write");
        }
    });
    sink += arc_sink.pos;

    ArduinoBytePrint byte_print;
    Print& byte_base = byte_print;
    bench("Arduino Print byte path", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = next_byte();
            barrier();
            expect(byte_base.write(payload.data(), payload.size()) == payload.size(), "Arduino byte write");
        }
    });
    sink += byte_print.pos;

    ArduinoBulkPrint bulk_print;
    Print& bulk_base = bulk_print;
    bench("Arduino Print bulk path", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = next_byte();
            barrier();
            expect(bulk_base.write(payload.data(), payload.size()) == payload.size(), "Arduino bulk write");
        }
    });
    sink += bulk_print.pos;

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
    -I"$ROOT/tests/host/stubs" \
    -I"$ROOT/components/arc/include" \
    "$BUILD/framework-print-compare.cpp" \
    "$BUILD/arduino/Print.cpp" \
    -o "$BUILD/framework-print-compare"

echo
"$BUILD/framework-print-compare"

echo
echo "raw ESP-IDF note: ESP-IDF driver/FreeRTOS components are present locally, but their real queue/ringbuffer/GPIO paths are target/FreeRTOS ports, not a host ABI. This script does not publish fake raw-IDF host timings for hardware drivers."
