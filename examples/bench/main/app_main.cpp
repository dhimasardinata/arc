#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <type_traits>

#include "aes/esp_aes.h"
#include "aes/esp_aes_gcm.h"
#include "arc/aes.hpp"
#include "arc/can.hpp"
#include "arc/coap.hpp"
#include "arc/copy.hpp"
#include "arc/dsp.hpp"
#include "arc/fanin.hpp"
#include "arc/mpsc.hpp"
#include "arc/mqtt.hpp"
#include "arc/ota.hpp"
#include "arc/probe.hpp"
#include "arc/rng.hpp"
#include "arc/seq.hpp"
#include "arc/sha.hpp"
#include "arc/spsc.hpp"
#include "arc/store.hpp"
#include "arc/stream.hpp"
#include "arc/temp.hpp"
#include "arc/time.hpp"
#include "arc/ws.hpp"

#include "esp_async_memcpy.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
#include "psa/crypto.h"

#ifdef ARC_BENCH_HAS_ARDUINO
#include "Arduino.h"
#include "Print.h"
#include "base64.h"
#undef word
#endif

namespace app {

inline constexpr char tag[] = "arc-bench";
inline constexpr std::uint32_t rounds = 20'000U;
inline constexpr std::uint32_t small_rounds = 4'000U;
inline constexpr std::uint32_t rng_rounds = 256U;
inline constexpr std::uint32_t dsp_rounds = 10'000U;
inline constexpr std::uint32_t telemetry_rounds = 512U;
inline constexpr std::uint32_t usage_rounds = 4'000U;
inline constexpr std::uint32_t can_rounds = 256U;
inline constexpr std::uint32_t flash_rounds = 16U;  // keep write-heavy NVS lanes from needlessly wearing flash
inline constexpr std::size_t batch = 15U;
inline constexpr std::size_t dsp_n = 256U;
inline constexpr char store_ns[] = "bench";
inline constexpr char store_blob_key[] = "control";
inline constexpr char store_name_key[] = "name";

volatile std::uint64_t sink{};

struct Result {
    const char* name;
    std::uint64_t ops;
    std::uint32_t cycles_op;
    std::uint32_t ns_op;
};

void print(const Result result)
{
    ESP_LOGI(
        tag,
        "%-34s %10llu ops %9u cyc/op %9u ns/op",
        result.name,
        static_cast<unsigned long long>(result.ops),
        static_cast<unsigned>(result.cycles_op),
        static_cast<unsigned>(result.ns_op));
}

void section(const char* const name)
{
    ESP_LOGI(tag, "%s", "");
    ESP_LOGI(tag, "== %s ==", name);
    ESP_LOGI(tag, "%-34s %10s     %9s     %9s", "lane", "ops", "cycles", "ns");
}

template <typename Fn>
Result measure(const char* const name, const std::uint64_t ops, Fn&& fn)
{
    const auto t0 = arc::Time::us();
    const auto mark = arc::Probe::now();
    fn();
    const auto cycles = mark.lap();
    const auto us = arc::Time::since(t0);
    return Result{
        .name = name,
        .ops = ops,
        .cycles_op = ops == 0U ? 0U : static_cast<std::uint32_t>(cycles / ops),
        .ns_op = ops == 0U ? 0U : static_cast<std::uint32_t>((us * 1000ULL) / ops),
    };
}

struct SinkStream {
    std::array<std::uint8_t, 64> data{};
    std::size_t pos{};
    std::uint64_t checksum{};

    arc::Status send_all(const void* const src, const std::size_t bytes) noexcept
    {
        const auto* in = static_cast<const std::uint8_t*>(src);
        for (std::size_t i = 0; i < bytes; ++i) {
            const auto value = in[i];
            data[(pos + i) & (data.size() - 1U)] = value;
            checksum += value;
        }
        pos += bytes;
        return arc::ok();
    }

    arc::Result<std::size_t> recv(void*, std::size_t) noexcept
    {
        return std::size_t{};
    }
};

struct FrameSource {
    std::size_t pos{};

    arc::Status send_all(const void*, std::size_t) noexcept
    {
        return arc::ok();
    }

    arc::Result<std::size_t> recv(void* const dst, const std::size_t bytes) noexcept
    {
        auto* out = static_cast<std::uint8_t*>(dst);
        const auto chunk = bytes > 11U ? 11U : bytes;
        for (std::size_t i = 0; i < chunk; ++i) {
            const auto frame = pos % 34U;
            if (frame == 0U) {
                out[i] = 0U;
            } else if (frame == 1U) {
                out[i] = 32U;
            } else {
                out[i] = static_cast<std::uint8_t>(frame - 2U);
            }
            ++pos;
        }
        return chunk;
    }
};

struct AsyncCopySignal {
    volatile std::uint32_t done{};
};

struct ControlWord {
    std::uint16_t pace;
    std::uint8_t mark;
    std::uint8_t flags;
};

static_assert(sizeof(ControlWord) == 4U);

struct TelemetryFrame {
    std::uint32_t seq;
    std::int32_t accel_x;
    std::int32_t accel_y;
    std::int32_t accel_z;
    std::uint16_t millivolts;
    std::uint16_t centi_celsius;
    std::uint8_t flags;
    std::uint8_t lane;
};

static_assert(std::is_trivially_copyable_v<TelemetryFrame>);

struct ControlSnapshot {
    std::uint32_t tick;
    std::int32_t error;
    std::int32_t effort;
    std::uint32_t flags;
};

struct ControlEvent {
    std::uint16_t code;
    std::uint16_t value;
    std::uint32_t tick;
};

using BenchCan = arc::Can<
    4,
    4,
    500'000,
    4,
    32,
    true,
    true>;

struct RawNvsHandle {
    nvs_handle_t raw{};

    ~RawNvsHandle()
    {
        if (raw != 0) {
            nvs_close(raw);
        }
    }

    [[nodiscard]] esp_err_t open(const char* const ns, const nvs_open_mode_t mode) noexcept
    {
        return nvs_open(ns, mode, &raw);
    }
};

[[nodiscard]] esp_err_t raw_store_save(
    const char* const ns,
    const char* const key,
    const ControlWord& value) noexcept
{
    RawNvsHandle handle{};
    auto err = handle.open(ns, NVS_READWRITE);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(handle.raw, key, &value, sizeof(value));
    if (err != ESP_OK) {
        return err;
    }

    return nvs_commit(handle.raw);
}

[[nodiscard]] esp_err_t raw_store_load(
    const char* const ns,
    const char* const key,
    ControlWord& value) noexcept
{
    RawNvsHandle handle{};
    auto err = handle.open(ns, NVS_READONLY);
    if (err != ESP_OK) {
        return err;
    }

    std::size_t bytes = sizeof(value);
    err = nvs_get_blob(handle.raw, key, &value, &bytes);
    if (err != ESP_OK) {
        return err;
    }

    return bytes == sizeof(value) ? ESP_OK : ESP_ERR_NVS_INVALID_LENGTH;
}

[[nodiscard]] esp_err_t raw_store_save_string(
    const char* const ns,
    const char* const key,
    const char* const value) noexcept
{
    RawNvsHandle handle{};
    auto err = handle.open(ns, NVS_READWRITE);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle.raw, key, value);
    if (err != ESP_OK) {
        return err;
    }

    return nvs_commit(handle.raw);
}

[[nodiscard]] esp_err_t raw_store_load_string(
    const char* const ns,
    const char* const key,
    const std::span<char> out,
    std::size_t* const chars = nullptr) noexcept
{
    RawNvsHandle handle{};
    auto err = handle.open(ns, NVS_READONLY);
    if (err != ESP_OK) {
        return err;
    }

    std::size_t bytes = out.size_bytes();
    err = nvs_get_str(handle.raw, key, out.data(), &bytes);
    if (err != ESP_OK) {
        return err;
    }

    if (chars != nullptr) {
        *chars = bytes == 0U ? 0U : bytes - 1U;
    }
    return ESP_OK;
}

IRAM_ATTR bool raw_copy_done(async_memcpy_handle_t, async_memcpy_event_t*, void* const ctx) noexcept
{
    auto* const signal = reinterpret_cast<AsyncCopySignal*>(ctx);
    signal->done = 1U;
    return false;
}

void init_psa()
{
    static bool ready{};
    if (!ready) {
        configASSERT(psa_crypto_init() == PSA_SUCCESS);
        ready = true;
    }
}

#ifdef ARC_BENCH_HAS_ARDUINO
struct ArduinoSinkPrint final : public Print {
    std::array<std::uint8_t, 64> data{};
    std::size_t pos{};
    std::uint64_t checksum{};

    std::size_t write(const std::uint8_t value) override
    {
        data[pos & (data.size() - 1U)] = value;
        checksum += value;
        ++pos;
        return 1U;
    }

    std::size_t write(const std::uint8_t* const buffer, const std::size_t size) override
    {
        for (std::size_t i = 0; i < size; ++i) {
            const auto value = buffer[i];
            data[(pos + i) & (data.size() - 1U)] = value;
            checksum += value;
        }
        pos += size;
        return size;
    }
};

void init_arduino()
{
    static bool ready{};
    if (!ready) {
        initArduino();
        ready = true;
    }
}
#endif

void bench_spsc()
{
    arc::Spsc<std::uint32_t, 16> q;
    std::uint64_t sum{};
    print(measure("spsc single", rounds * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            configASSERT(q.try_push(i));
            std::uint32_t out{};
            configASSERT(q.try_pop(out));
            sum += out;
        }
    }));
    sink += sum;

    arc::Spsc<std::uint32_t, 16> bq;
    std::array<std::uint32_t, batch> in{};
    std::array<std::uint32_t, batch> out{};
    sum = 0U;
    print(measure("spsc batch", rounds * 2ULL, [&]() {
        for (std::uint32_t base = 0; base < rounds; base += batch) {
            const auto count = (rounds - base) < batch ? (rounds - base) : batch;
            for (std::uint32_t i = 0; i < count; ++i) {
                in[i] = base + i;
            }
            configASSERT(bq.push(std::span{in}.first(count)) == count);
            configASSERT(bq.pop(std::span{out}.first(count)) == count);
            for (std::uint32_t i = 0; i < count; ++i) {
                sum += out[i];
            }
        }
    }));
    sink += sum;
}

template <typename Queue>
void bench_mpsc_one(const char* const name)
{
    Queue q;
    std::uint64_t sum{};
    print(measure(name, rounds * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            configASSERT(q.try_push(i));
            std::uint32_t out{};
            configASSERT(q.try_pop(out));
            sum += out;
        }
    }));
    sink += sum;
}

void bench_fanin()
{
    arc::Fanin<std::uint32_t, 16, 4> fan;
    std::uint64_t sum{};
    print(measure("fanin single", rounds * 8ULL, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            configASSERT(fan.try_push<0>(i));
            configASSERT(fan.try_push<1>(i));
            configASSERT(fan.try_push<2>(i));
            configASSERT(fan.try_push<3>(i));
            for (std::uint32_t n = 0; n < 4U; ++n) {
                std::uint32_t out{};
                configASSERT(fan.try_pop(out));
                sum += out;
            }
        }
    }));
    sink += sum;

    arc::Fanin<std::uint32_t, 16, 4> batch_fan;
    std::array<std::uint32_t, batch> in{};
    std::array<std::uint32_t, batch * 4U> out{};
    sum = 0U;
    print(measure("fanin batch", rounds * 8ULL, [&]() {
        for (std::uint32_t base = 0; base < rounds; base += batch) {
            const auto count = (rounds - base) < batch ? (rounds - base) : batch;
            for (std::uint32_t i = 0; i < count; ++i) {
                in[i] = base + i;
            }
            configASSERT(batch_fan.push<0>(std::span{in}.first(count)) == count);
            configASSERT(batch_fan.push<1>(std::span{in}.first(count)) == count);
            configASSERT(batch_fan.push<2>(std::span{in}.first(count)) == count);
            configASSERT(batch_fan.push<3>(std::span{in}.first(count)) == count);
            const auto got = batch_fan.pop(std::span{out}.first(count * 4U));
            configASSERT(got == count * 4U);
            for (std::size_t i = 0; i < got; ++i) {
                sum += out[i];
            }
        }
    }));
    sink += sum;
}

void bench_seqreg()
{
    struct Snapshot {
        std::uint32_t a;
        std::uint32_t b;
        std::uint64_t c;
    };

    arc::SeqReg<Snapshot> reg;
    std::uint64_t sum{};
    print(measure("seqreg write/read", rounds * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            reg.write_unmasked(Snapshot{.a = i, .b = ~i, .c = static_cast<std::uint64_t>(i) * 3U});
            const auto out = reg.read();
            configASSERT(out.b == ~out.a);
            sum += out.a;
        }
    }));
    sink += sum;
}

void bench_stream()
{
    SinkStream stream;
    std::array<std::uint8_t, 32> payload{};
    std::array<std::uint8_t, 40> frame{};
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i);
    }

    print(measure("stream write", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::net::Stream::write(stream, std::span(payload)).has_value());
        }
    }));
    sink += stream.checksum;

    print(measure("stream frame write", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::net::Stream::write_frame16(stream, std::span(payload)).has_value());
        }
    }));
    sink += stream.checksum;

    print(measure("stream frame encode", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            const auto encoded = arc::net::Stream::frame16(std::span(frame), std::span(payload));
            configASSERT(encoded.has_value());
            sink += (*encoded)[i % encoded->size()];
        }
    }));

    FrameSource source;
    print(measure("stream frame read", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            std::array<std::uint8_t, 32> out{};
            const auto got = arc::net::Stream::read_frame16(source, std::span(out));
            configASSERT(got.has_value() && *got == out.size());
            sink += out[i & 31U];
        }
    }));
}

void bench_codecs()
{
    std::array<std::uint8_t, 256> buffer{};
    std::array<std::uint8_t, 8> payload{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

    print(measure("mqtt codec", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            const auto frame = arc::net::Mqtt::publish(buffer, "arc/bench", std::span(payload), static_cast<std::uint16_t>(i));
            configASSERT(frame.has_value());
            const auto parsed = arc::net::Mqtt::parse(*frame);
            configASSERT(parsed.has_value());
            const auto view = arc::net::Mqtt::view(*parsed);
            configASSERT(view.has_value());
            sink += view->payload[0];
        }
    }));

    print(measure("websocket codec", small_rounds, [&]() {
        std::array<std::uint8_t, 32> scratch{};
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            const auto frame = arc::net::Ws::binary(buffer, std::span(payload), true, i);
            configASSERT(frame.has_value());
            const auto parsed = arc::net::Ws::parse(*frame, scratch);
            configASSERT(parsed.has_value());
            sink += parsed->payload[0];
        }
    }));

    print(measure("coap codec", small_rounds, [&]() {
        const std::array options{
            arc::net::Coap::text(static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "bench"),
        };
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            const auto frame = arc::net::Coap::message(
                buffer,
                arc::net::CoapType::confirmable,
                static_cast<std::uint8_t>(arc::net::CoapCode::post),
                static_cast<std::uint16_t>(i),
                {},
                std::span(options),
                std::span(payload));
            configASSERT(frame.has_value());
            const auto parsed = arc::net::Coap::parse(*frame);
            configASSERT(parsed.has_value());
            sink += parsed->payload[0];
        }
    }));
}

void bench_usage()
{
    arc::Spsc<TelemetryFrame, 64> telemetry;
    std::array<TelemetryFrame, 31> produced{};
    std::array<TelemetryFrame, 31> consumed{};
    std::uint64_t telemetry_sum{};

    print(measure("usage telemetry bridge", usage_rounds * 2ULL, [&]() {
        for (std::uint32_t base = 0; base < usage_rounds; base += produced.size()) {
            const auto count = (usage_rounds - base) < produced.size() ? (usage_rounds - base) : produced.size();
            for (std::uint32_t i = 0; i < count; ++i) {
                const auto seq = base + i;
                produced[i] = TelemetryFrame{
                    .seq = seq,
                    .accel_x = static_cast<std::int32_t>((seq * 3U) & 0x3ffU) - 512,
                    .accel_y = static_cast<std::int32_t>((seq * 5U) & 0x3ffU) - 512,
                    .accel_z = static_cast<std::int32_t>((seq * 7U) & 0x3ffU) - 512,
                    .millivolts = static_cast<std::uint16_t>(3300U + (seq & 31U)),
                    .centi_celsius = static_cast<std::uint16_t>(2450U + (seq & 63U)),
                    .flags = static_cast<std::uint8_t>(seq & 3U),
                    .lane = static_cast<std::uint8_t>((seq >> 2U) & 3U),
                };
            }

            configASSERT(telemetry.push(std::span{produced}.first(count)) == count);
            configASSERT(telemetry.pop(std::span{consumed}.first(count)) == count);
            for (std::uint32_t i = 0; i < count; ++i) {
                configASSERT(consumed[i].seq == base + i);
                telemetry_sum += static_cast<std::uint64_t>(consumed[i].millivolts);
                telemetry_sum += static_cast<std::uint64_t>(consumed[i].centi_celsius);
                telemetry_sum += consumed[i].flags;
            }
        }
    }));
    sink += telemetry_sum;

    arc::SeqReg<ControlSnapshot> snapshot;
    arc::Fanin<ControlEvent, 16, 3> events;
    std::array<int, 32> samples{};
    std::array<int, 32> work{};
    arc::dsp::Fir<int, 8>::State filter{};
    constexpr arc::dsp::Fir<int, 8>::Coeffs taps{1, -1, 2, -2, 3, -3, 4, -4};
    std::uint64_t control_sum{};

    print(measure("usage control tick", usage_rounds, [&]() {
        for (std::uint32_t tick = 0; tick < usage_rounds; ++tick) {
            for (std::size_t i = 0; i < samples.size(); ++i) {
                samples[i] = static_cast<int>((tick + i) & 31U) - 16;
            }

            arc::dsp::scale(work.data(), samples.data(), 2, work.size());
            arc::dsp::mac(work.data(), samples.data(), -1, work.size());
            const auto peak = arc::dsp::peak(work.data(), work.size());
            const auto effort = arc::dsp::Fir<int, 8>::step(filter, taps, peak);
            snapshot.write_unmasked(ControlSnapshot{
                .tick = tick,
                .error = peak,
                .effort = effort,
                .flags = tick & 0x0fU,
            });

            if ((tick & 1U) == 0U) {
                configASSERT(events.try_push<0>(ControlEvent{.code = 1U, .value = static_cast<std::uint16_t>(peak), .tick = tick}));
            }
            if ((tick & 3U) == 0U) {
                configASSERT(events.try_push<1>(ControlEvent{.code = 2U, .value = static_cast<std::uint16_t>(effort), .tick = tick}));
            }
            if ((tick & 7U) == 0U) {
                configASSERT(events.try_push<2>(ControlEvent{.code = 3U, .value = static_cast<std::uint16_t>(tick), .tick = tick}));
            }

            ControlEvent event{};
            while (events.try_pop(event)) {
                control_sum += static_cast<std::uint64_t>(event.code);
                control_sum += event.value;
                control_sum += event.tick;
            }

            const auto latest = snapshot.read();
            configASSERT(latest.tick == tick);
            control_sum += static_cast<std::uint64_t>(latest.flags);
        }
    }));
    sink += control_sum;

    SinkStream stream;
    std::array<std::uint8_t, 256> packet{};
    std::array<std::uint8_t, 64> frame{};
    std::array<std::uint8_t, 32> scratch{};
    std::array<std::uint8_t, 32> payload{};
    const std::array options{
        arc::net::Coap::text(static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "telemetry"),
    };
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i);
    }

    print(measure("usage protocol bundle", usage_rounds * 4ULL, [&]() {
        for (std::uint32_t i = 0; i < usage_rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);

            const auto mqtt = arc::net::Mqtt::publish(packet, "arc/node/telemetry", std::span(payload), static_cast<std::uint16_t>(i));
            configASSERT(mqtt.has_value());
            const auto mqtt_packet = arc::net::Mqtt::parse(*mqtt);
            configASSERT(mqtt_packet.has_value());
            const auto mqtt_view = arc::net::Mqtt::view(*mqtt_packet);
            configASSERT(mqtt_view.has_value() && mqtt_view->payload[0] == payload[0]);

            const auto coap = arc::net::Coap::message(
                packet,
                arc::net::CoapType::nonconfirmable,
                static_cast<std::uint8_t>(arc::net::CoapCode::post),
                static_cast<std::uint16_t>(i),
                {},
                std::span(options),
                std::span(payload));
            configASSERT(coap.has_value());
            const auto coap_packet = arc::net::Coap::parse(*coap);
            configASSERT(coap_packet.has_value() && coap_packet->payload[0] == payload[0]);

            const auto ws = arc::net::Ws::binary(packet, std::span(payload), true, i);
            configASSERT(ws.has_value());
            const auto ws_frame = arc::net::Ws::parse(*ws, scratch);
            configASSERT(ws_frame.has_value() && ws_frame->payload[0] == payload[0]);

            const auto encoded = arc::net::Stream::frame16(std::span(frame), std::span(payload));
            configASSERT(encoded.has_value());
            configASSERT(arc::net::Stream::write(stream, *encoded).has_value());
            sink += mqtt_view->payload[0];
            sink += coap_packet->payload[0];
            sink += ws_frame->payload[0];
        }
    }));
    sink += stream.pos + stream.checksum;
}

void bench_dsp()
{
    static std::array<int, dsp_n> lhs{};
    static std::array<int, dsp_n> rhs{};
    static std::array<int, dsp_n> out{};
    for (std::size_t i = 0; i < dsp_n; ++i) {
        lhs[i] = static_cast<int>(i % 17U) - 8;
        rhs[i] = static_cast<int>(i % 11U) - 5;
        out[i] = 0;
    }

    std::int64_t sum{};
    print(measure("dsp dot", dsp_rounds * dsp_n, [&]() {
        for (std::uint32_t i = 0; i < dsp_rounds; ++i) {
            sum += arc::dsp::dot(lhs.data(), rhs.data(), lhs.size());
        }
    }));
    sink += static_cast<std::uint64_t>(sum);

    print(measure("dsp scale/mac/peak", dsp_rounds * dsp_n * 3ULL, [&]() {
        for (std::uint32_t i = 0; i < dsp_rounds; ++i) {
            arc::dsp::scale(out.data(), lhs.data(), 2, out.size());
            arc::dsp::mac(out.data(), rhs.data(), 3, out.size());
            sink += static_cast<std::uint64_t>(arc::dsp::peak(out.data(), out.size()));
        }
    }));

    arc::dsp::Fir<int, 8>::State state{};
    constexpr arc::dsp::Fir<int, 8>::Coeffs taps{1, -1, 2, -2, 3, -3, 4, -4};
    print(measure("dsp fir", dsp_rounds * dsp_n, [&]() {
        for (std::uint32_t r = 0; r < dsp_rounds; ++r) {
            for (std::size_t i = 0; i < dsp_n; ++i) {
                sink += static_cast<std::uint64_t>(arc::dsp::Fir<int, 8>::step(state, taps, lhs[i]));
            }
        }
    }));
}

void bench_copy()
{
    static std::array<std::uint8_t, 256> src{};
    static std::array<std::uint8_t, 256> dst{};
    for (std::size_t i = 0; i < src.size(); ++i) {
        src[i] = static_cast<std::uint8_t>(i * 3U);
    }

    print(measure("std memcpy 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            src[0] = static_cast<std::uint8_t>(i);
            std::memcpy(dst.data(), src.data(), src.size());
            sink += dst[i & 255U];
        }
    }));

    async_memcpy_config_t raw_config{};
    raw_config.backlog = 4U;
    raw_config.weight = 0U;
    raw_config.dma_burst_size = 64U;
    async_memcpy_handle_t raw_driver{};
    ESP_ERROR_CHECK(esp_async_memcpy_install(&raw_config, &raw_driver));
    print(measure("idf async memcpy 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            src[0] = static_cast<std::uint8_t>(i);
            AsyncCopySignal signal{};
            ESP_ERROR_CHECK(esp_async_memcpy(raw_driver, dst.data(), src.data(), src.size(), raw_copy_done, &signal));
            while (signal.done == 0U) {
            }
            sink += dst[i & 255U];
        }
    }));
    ESP_ERROR_CHECK(esp_async_memcpy_uninstall(raw_driver));

    ESP_ERROR_CHECK(arc::Copy<>::init());
    print(measure("copy dma 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            src[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::Copy<>::copy(std::span(dst), std::span(src)) == ESP_OK);
            sink += dst[i & 255U];
        }
    }));
}

void bench_silicon()
{
    init_psa();

    std::array<std::uint8_t, 256> data{};
    std::array<std::uint8_t, 32> sum{};
    std::array<std::uint8_t, 16> key{};
    std::array<std::uint8_t, 16> block{};
    std::array<std::uint8_t, 16> crypt{};
    std::array<std::uint8_t, 16> iv{};
    std::array<std::uint8_t, 16> auth_tag{};
    std::array<std::uint8_t, 16> stream{};
    std::array<std::uint8_t, 64> plain{};
    std::array<std::uint8_t, 64> cipher{};
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i);
    }
    for (std::size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<std::uint8_t>(i * 7U);
        block[i] = static_cast<std::uint8_t>(i * 13U);
        iv[i] = static_cast<std::uint8_t>(0xa0U + i);
    }
    for (std::size_t i = 0; i < plain.size(); ++i) {
        plain[i] = static_cast<std::uint8_t>(i * 5U);
    }

    print(measure("rng word", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            sink += arc::Rng::word();
        }
    }));

    print(measure("idf esp_random word", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            sink += esp_random();
        }
    }));

    print(measure("rng fill 256B", rng_rounds, [&]() {
        for (std::uint32_t i = 0; i < rng_rounds; ++i) {
            arc::Rng::fill(std::span(data));
            sink += data[i & 255U];
        }
    }));

    print(measure("idf esp_fill_random", rng_rounds, [&]() {
        for (std::uint32_t i = 0; i < rng_rounds; ++i) {
            esp_fill_random(data.data(), data.size());
            sink += data[i & 255U];
        }
    }));

    print(measure("sha256 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            data[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::Sha::sum(SHA2_256, data.data(), data.size(), sum.data(), sum.size()) == ESP_OK);
            sink += sum[0];
        }
    }));

    print(measure("idf psa sha256 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            data[0] = static_cast<std::uint8_t>(i);
            std::size_t out_len{};
            configASSERT(psa_hash_compute(
                             PSA_ALG_SHA_256,
                             data.data(),
                             data.size(),
                             sum.data(),
                             sum.size(),
                             &out_len) == PSA_SUCCESS);
            configASSERT(out_len == sum.size());
            sink += sum[0];
        }
    }));

    arc::Aes aes;
    ESP_ERROR_CHECK(aes.set(std::span(key)));
    esp_aes_context raw_aes{};
    esp_aes_init(&raw_aes);
    configASSERT(esp_aes_setkey(&raw_aes, key.data(), static_cast<unsigned>(key.size() * 8U)) == 0);
    print(measure("aes ecb block", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            block[0] = static_cast<std::uint8_t>(i);
            configASSERT(aes.block(arc::Aes::Way::enc, block.data(), crypt.data()) == ESP_OK);
            sink += crypt[0];
        }
    }));

    print(measure("idf esp_aes ecb", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            block[0] = static_cast<std::uint8_t>(i);
            configASSERT(esp_aes_crypt_ecb(&raw_aes, ESP_AES_ENCRYPT, block.data(), crypt.data()) == 0);
            sink += crypt[0];
        }
    }));

    print(measure("aes cbc 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto work_iv = iv;
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(aes.cbc(arc::Aes::Way::enc, plain.data(), cipher.data(), plain.size(), work_iv.data()) == ESP_OK);
            sink += cipher[i & 63U];
        }
    }));

    print(measure("idf esp_aes cbc", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto work_iv = iv;
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(esp_aes_crypt_cbc(
                             &raw_aes,
                             ESP_AES_ENCRYPT,
                             plain.size(),
                             work_iv.data(),
                             plain.data(),
                             cipher.data()) == 0);
            sink += cipher[i & 63U];
        }
    }));

    print(measure("aes ctr 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto nonce = iv;
            std::size_t offset{};
            plain[0] = static_cast<std::uint8_t>(i);
            std::memset(stream.data(), 0, stream.size());
            configASSERT(aes.ctr(plain.data(), cipher.data(), plain.size(), offset, nonce.data(), stream.data()) == ESP_OK);
            sink += cipher[i & 63U];
        }
    }));

    print(measure("idf esp_aes ctr", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto nonce = iv;
            std::size_t offset{};
            plain[0] = static_cast<std::uint8_t>(i);
            std::memset(stream.data(), 0, stream.size());
            configASSERT(esp_aes_crypt_ctr(
                             &raw_aes,
                             plain.size(),
                             &offset,
                             nonce.data(),
                             stream.data(),
                             plain.data(),
                             cipher.data()) == 0);
            sink += cipher[i & 63U];
        }
    }));

    print(measure("aes ofb 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto work_iv = iv;
            std::size_t offset{};
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(aes.ofb(plain.data(), cipher.data(), plain.size(), offset, work_iv.data()) == ESP_OK);
            sink += cipher[i & 63U];
        }
    }));

    print(measure("idf esp_aes ofb", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto work_iv = iv;
            std::size_t offset{};
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(esp_aes_crypt_ofb(
                             &raw_aes,
                             plain.size(),
                             &offset,
                             work_iv.data(),
                             plain.data(),
                             cipher.data()) == 0);
            sink += cipher[i & 63U];
        }
    }));

    arc::Gcm gcm;
    ESP_ERROR_CHECK(gcm.set(std::span(key)));
    esp_gcm_context raw_gcm{};
    esp_aes_gcm_init(&raw_gcm);
    configASSERT(esp_aes_gcm_setkey(
                     &raw_gcm,
                     arc::Gcm::cipher,
                     key.data(),
                     static_cast<unsigned>(key.size() * 8U)) == 0);
    print(measure("aes gcm seal 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(gcm.seal(plain.data(), cipher.data(), plain.size(), iv.data(), 12U, data.data(), 16U, auth_tag.data()) == ESP_OK);
            sink += static_cast<std::uint64_t>(cipher[i & 63U]) + static_cast<std::uint64_t>(auth_tag[0]);
        }
    }));

    print(measure("idf esp_aes gcm", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(esp_aes_gcm_crypt_and_tag(
                             &raw_gcm,
                             ESP_AES_ENCRYPT,
                             plain.size(),
                             iv.data(),
                             12U,
                             data.data(),
                             16U,
                             plain.data(),
                             cipher.data(),
                             auth_tag.size(),
                             auth_tag.data()) == 0);
            sink += static_cast<std::uint64_t>(cipher[i & 63U]) + static_cast<std::uint64_t>(auth_tag[0]);
        }
    }));

    esp_aes_gcm_free(&raw_gcm);
    esp_aes_free(&raw_aes);
}

void bench_temp()
{
    using Die = arc::Temp<-10, 80>;
    Die::start();
    auto* const sensor = Die::native();
    float value = 0.0F;

    print(measure("temp read celsius", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            configASSERT(Die::read(value) == ESP_OK);
            sink += static_cast<std::uint64_t>((value + 100.0F) * 1000.0F);
        }
    }));

    print(measure("idf temp celsius", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            configASSERT(temperature_sensor_get_celsius(sensor, &value) == ESP_OK);
            sink += static_cast<std::uint64_t>((value + 100.0F) * 1000.0F);
        }
    }));
}

void bench_ota()
{
    print(measure("ota inspect", telemetry_rounds * 5ULL, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            const auto* const running = arc::Ota::running();
            const auto* const boot = arc::Ota::boot();
            const auto* const next = arc::Ota::next();
            configASSERT(running != nullptr);

            esp_ota_img_states_t state{};
            configASSERT(arc::Ota::state(running, state) == ESP_OK);

            esp_err_t pending_err = ESP_OK;
            const bool pending = arc::Ota::pending_verify(&pending_err);
            configASSERT(pending_err == ESP_OK);

            sink += static_cast<std::uint64_t>(running->address);
            sink += static_cast<std::uint64_t>(boot != nullptr ? boot->address : 0U);
            sink += static_cast<std::uint64_t>(next != nullptr ? next->address : 0U);
            sink += static_cast<std::uint64_t>(state);
            sink += pending ? 1U : 0U;
        }
    }));

    print(measure("idf ota inspect", telemetry_rounds * 5ULL, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            const auto* const running = esp_ota_get_running_partition();
            const auto* const boot = esp_ota_get_boot_partition();
            const auto* const next = esp_ota_get_next_update_partition(nullptr);
            configASSERT(running != nullptr);

            esp_ota_img_states_t state{};
            configASSERT(esp_ota_get_state_partition(running, &state) == ESP_OK);
            const bool pending = state == ESP_OTA_IMG_PENDING_VERIFY;

            sink += static_cast<std::uint64_t>(running->address);
            sink += static_cast<std::uint64_t>(boot != nullptr ? boot->address : 0U);
            sink += static_cast<std::uint64_t>(next != nullptr ? next->address : 0U);
            sink += static_cast<std::uint64_t>(state);
            sink += pending ? 1U : 0U;
        }
    }));
}

void bench_store()
{
    ESP_ERROR_CHECK(arc::Store::boot());

    ControlWord control{
        .pace = 2000U,
        .mark = 0x31U,
        .flags = 0x01U,
    };
    ControlWord loaded{};
    std::array<char, 16> label{"arc-bench-a"};
    std::array<char, 16> loaded_label{};
    std::size_t chars{};

    ESP_ERROR_CHECK(arc::Store::save(store_ns, store_blob_key, control));
    ESP_ERROR_CHECK(arc::Store::save_string(store_ns, store_name_key, label.data()));

    print(measure("store save blob", flash_rounds, [&]() {
        for (std::uint32_t i = 0; i < flash_rounds; ++i) {
            control.mark = static_cast<std::uint8_t>(0x31U + i);
            control.flags ^= 0x01U;
            configASSERT(arc::Store::save(store_ns, store_blob_key, control) == ESP_OK);
            sink += static_cast<std::uint64_t>(control.mark) + static_cast<std::uint64_t>(control.flags);
        }
    }));

    print(measure("idf nvs save blob", flash_rounds, [&]() {
        for (std::uint32_t i = 0; i < flash_rounds; ++i) {
            control.mark = static_cast<std::uint8_t>(0x51U + i);
            control.flags ^= 0x01U;
            configASSERT(raw_store_save(store_ns, store_blob_key, control) == ESP_OK);
            sink += static_cast<std::uint64_t>(control.mark) + static_cast<std::uint64_t>(control.flags);
        }
    }));

    print(measure("store load blob", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            configASSERT(arc::Store::load(store_ns, store_blob_key, loaded) == ESP_OK);
            sink += static_cast<std::uint64_t>(loaded.pace);
            sink += static_cast<std::uint64_t>(loaded.mark);
            sink += static_cast<std::uint64_t>(loaded.flags);
        }
    }));

    print(measure("idf nvs load blob", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            configASSERT(raw_store_load(store_ns, store_blob_key, loaded) == ESP_OK);
            sink += static_cast<std::uint64_t>(loaded.pace);
            sink += static_cast<std::uint64_t>(loaded.mark);
            sink += static_cast<std::uint64_t>(loaded.flags);
        }
    }));

    print(measure("store save string", flash_rounds, [&]() {
        for (std::uint32_t i = 0; i < flash_rounds; ++i) {
            label[10] = static_cast<char>('a' + static_cast<int>(i & 15U));
            configASSERT(arc::Store::save_string(store_ns, store_name_key, label.data()) == ESP_OK);
            sink += static_cast<std::uint8_t>(label[10]);
        }
    }));

    print(measure("idf nvs save string", flash_rounds, [&]() {
        for (std::uint32_t i = 0; i < flash_rounds; ++i) {
            label[10] = static_cast<char>('p' + static_cast<int>(i & 7U));
            configASSERT(raw_store_save_string(store_ns, store_name_key, label.data()) == ESP_OK);
            sink += static_cast<std::uint8_t>(label[10]);
        }
    }));

    print(measure("store load string", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            chars = 0U;
            configASSERT(arc::Store::load_string(store_ns, store_name_key, std::span<char>{loaded_label}, &chars) == ESP_OK);
            configASSERT(chars != 0U);
            sink += static_cast<std::uint8_t>(loaded_label[chars - 1U]);
        }
    }));

    print(measure("idf nvs load string", telemetry_rounds, [&]() {
        for (std::uint32_t i = 0; i < telemetry_rounds; ++i) {
            chars = 0U;
            configASSERT(raw_store_load_string(store_ns, store_name_key, std::span<char>{loaded_label}, &chars) == ESP_OK);
            configASSERT(chars != 0U);
            sink += static_cast<std::uint8_t>(loaded_label[chars - 1U]);
        }
    }));

    static_cast<void>(arc::Store::erase(store_ns, store_blob_key));
    static_cast<void>(arc::Store::erase(store_ns, store_name_key));
}

void bench_can()
{
    BenchCan::start();
    BenchCan::Frame stale{};
    while (BenchCan::recv(stale)) {
    }

    std::array<std::uint8_t, 8> payload{0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    std::uint64_t sum{};
    print(measure("can selftest txrx", can_rounds * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < can_rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            auto tx = BenchCan::frame(0x123U, std::span<const std::uint8_t>{payload});
            configASSERT(BenchCan::send_wait(tx, 1000) == ESP_OK);

            BenchCan::Frame rx{};
            while (!BenchCan::recv(rx)) {
            }

            configASSERT(rx.header.id == 0x123U);
            configASSERT(rx.bytes == static_cast<std::uint16_t>(payload.size()));
            configASSERT(rx.data[0] == payload[0]);
            sum += rx.data[0];
        }
    }));
    sink += sum;
    sink += static_cast<std::uint64_t>(BenchCan::done()) + static_cast<std::uint64_t>(BenchCan::rx());
    BenchCan::stop();
}

#ifdef ARC_BENCH_HAS_ARDUINO
void bench_arduino()
{
    init_arduino();

    std::array<std::uint8_t, 32> payload{};
    std::array<std::uint8_t, 16> nonce{};
    std::array<char, 32> arc_key{};
    std::array<unsigned char, 32> idf_key{};
    const std::array<std::uint8_t, 2> frame_header{
        0U,
        static_cast<std::uint8_t>(payload.size()),
    };
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i);
    }
    for (std::size_t i = 0; i < nonce.size(); ++i) {
        nonce[i] = static_cast<std::uint8_t>(i * 7U);
    }

    SinkStream arc_write{};
    print(measure("arc stream write 32B", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::net::Stream::write(arc_write, std::span(payload)).has_value());
        }
    }));
    sink += arc_write.pos + arc_write.checksum;

    ArduinoSinkPrint arduino_write;
    print(measure("arduino write 32B", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arduino_write.write(payload.data(), payload.size()) == payload.size());
        }
    }));
    sink += arduino_write.pos + arduino_write.checksum;

    SinkStream arc_frame{};
    print(measure("arc frame16 32B", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arc::net::Stream::write_frame16(arc_frame, std::span(payload)).has_value());
        }
    }));
    sink += arc_frame.pos + arc_frame.checksum;

    ArduinoSinkPrint arduino_frame;
    print(measure("arduino frame16 32B", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            configASSERT(arduino_frame.write(frame_header.data(), frame_header.size()) == frame_header.size());
            configASSERT(arduino_frame.write(payload.data(), payload.size()) == payload.size());
        }
    }));
    sink += arduino_frame.pos + arduino_frame.checksum;

    ArduinoSinkPrint arduino_numbers;
    print(measure("arduino print u32", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(i);
            const auto value = static_cast<unsigned long>((static_cast<std::uint32_t>(payload[0]) << 16U) | i);
            configASSERT(arduino_numbers.print(value, DEC) != 0U);
        }
    }));
    sink += arduino_numbers.pos + arduino_numbers.checksum;

    print(measure("arc ws key b64", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            nonce[i & 15U] = static_cast<std::uint8_t>(i);
            const auto out = arc::net::Ws::key(arc_key, std::span<const std::uint8_t>{nonce});
            configASSERT(out.has_value() && out->size() == 24U);
            sink += static_cast<std::uint8_t>((*out)[0]);
        }
    }));

    print(measure("idf mbedtls b64", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            nonce[i & 15U] = static_cast<std::uint8_t>(i);
            std::size_t out_len{};
            configASSERT(mbedtls_base64_encode(
                             idf_key.data(),
                             idf_key.size(),
                             &out_len,
                             nonce.data(),
                             nonce.size()) == 0);
            configASSERT(out_len == 24U);
            sink += idf_key[0];
        }
    }));

    print(measure("arduino b64", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            nonce[i & 15U] = static_cast<std::uint8_t>(i);
            const String out = base64::encode(nonce.data(), nonce.size());
            configASSERT(out.length() == 24U);
            sink += static_cast<std::uint8_t>(out.c_str()[0]);
        }
    }));
}
#endif

void run()
{
    ESP_LOGI(tag, "real ESP32-S3 benchmark start");
    ESP_LOGI(tag, "scope=no external fixture; external-device buses still need a wiring fixture, internal/self-test lanes are included");
    ESP_LOGI(tag, "compare=Arc + raw ESP-IDF silicon paths");
    ESP_LOGI(tag, "system=temp + ota + nvs + twai self-test loopback");
#ifdef ARC_BENCH_HAS_ARDUINO
    ESP_LOGI(tag, "compare+=Arduino-ESP32 component paths");
#else
    ESP_LOGI(tag, "compare+=Arduino-ESP32 skipped; clone arduino-esp32/ or set ARC_ARDUINO_PATH to enable it");
#endif
    section("core lanes");
    bench_spsc();
    bench_mpsc_one<arc::Mpsc<std::uint32_t, 16>>("mpsc single");
    bench_mpsc_one<arc::DenseMpsc<std::uint32_t, 16>>("dense mpsc single");
    bench_fanin();
    bench_seqreg();

    section("streams and codecs");
    bench_stream();
    bench_codecs();

    section("critical usage mixes");
    bench_usage();

    section("dsp and memory");
    bench_dsp();
    bench_copy();

    section("silicon accelerators");
    bench_silicon();

    section("system services");
    bench_temp();
    bench_ota();
    bench_store();
    bench_can();
#ifdef ARC_BENCH_HAS_ARDUINO
    section("arduino component compare");
    bench_arduino();
#endif
    ESP_LOGI(tag, "real ESP32-S3 benchmark done sink=%llu", static_cast<unsigned long long>(sink));
}

}  // namespace app

extern "C" void app_main()
{
    app::run();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
