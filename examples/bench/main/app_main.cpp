#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "arc/aes.hpp"
#include "arc/coap.hpp"
#include "arc/copy.hpp"
#include "arc/dsp.hpp"
#include "arc/fanin.hpp"
#include "arc/mpsc.hpp"
#include "arc/mqtt.hpp"
#include "arc/probe.hpp"
#include "arc/rng.hpp"
#include "arc/seq.hpp"
#include "arc/sha.hpp"
#include "arc/spsc.hpp"
#include "arc/stream.hpp"
#include "arc/time.hpp"
#include "arc/ws.hpp"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace app {

inline constexpr char tag[] = "arc-bench";
inline constexpr std::uint32_t rounds = 20'000U;
inline constexpr std::uint32_t small_rounds = 4'000U;
inline constexpr std::uint32_t dsp_rounds = 10'000U;
inline constexpr std::size_t batch = 255U;
inline constexpr std::size_t dsp_n = 256U;

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
        "%-28s ops=%llu cycles/op=%u ns/op=%u",
        result.name,
        static_cast<unsigned long long>(result.ops),
        static_cast<unsigned>(result.cycles_op),
        static_cast<unsigned>(result.ns_op));
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
    std::array<std::uint8_t, 4096> data{};
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

void bench_spsc()
{
    arc::Spsc<std::uint32_t, 512> q;
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

    arc::Spsc<std::uint32_t, 512> bq;
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
    arc::Fanin<std::uint32_t, 512, 4> fan;
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

    arc::Fanin<std::uint32_t, 512, 4> batch_fan;
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

    print(measure("rng fill 256B", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            arc::Rng::fill(std::span(data));
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

    arc::Aes aes;
    ESP_ERROR_CHECK(aes.set(std::span(key)));
    print(measure("aes ecb block", small_rounds, [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            block[0] = static_cast<std::uint8_t>(i);
            configASSERT(aes.block(arc::Aes::Way::enc, block.data(), crypt.data()) == ESP_OK);
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

    print(measure("aes ofb 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            auto work_iv = iv;
            std::size_t offset{};
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(aes.ofb(plain.data(), cipher.data(), plain.size(), offset, work_iv.data()) == ESP_OK);
            sink += cipher[i & 63U];
        }
    }));

    arc::Gcm gcm;
    ESP_ERROR_CHECK(gcm.set(std::span(key)));
    print(measure("aes gcm seal 64B", small_rounds * plain.size(), [&]() {
        for (std::uint32_t i = 0; i < small_rounds; ++i) {
            plain[0] = static_cast<std::uint8_t>(i);
            configASSERT(gcm.seal(plain.data(), cipher.data(), plain.size(), iv.data(), 12U, data.data(), 16U, auth_tag.data()) == ESP_OK);
            sink += static_cast<std::uint64_t>(cipher[i & 63U]) + static_cast<std::uint64_t>(auth_tag[0]);
        }
    }));
}

void run()
{
    ESP_LOGI(tag, "real ESP32-S3 benchmark start");
    ESP_LOGI(tag, "scope=no external fixture; hardware peripherals with wiring are intentionally excluded");
    bench_spsc();
    bench_mpsc_one<arc::Mpsc<std::uint32_t, 512>>("mpsc single");
    bench_mpsc_one<arc::DenseMpsc<std::uint32_t, 512>>("dense mpsc single");
    bench_fanin();
    bench_seqreg();
    bench_stream();
    bench_codecs();
    bench_dsp();
    bench_copy();
    bench_silicon();
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
