#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <span>
#include <thread>

#include "arc/coap.hpp"
#include "arc/dsp.hpp"
#include "arc/fanin.hpp"
#include "arc/mpsc.hpp"
#include "arc/mqtt.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/stream.hpp"
#include "arc/ws.hpp"

namespace {

using Clock = std::chrono::steady_clock;

volatile std::uint64_t sink{};
volatile std::uint32_t seed{0x1234'5678U};

std::uint32_t next_seed() noexcept
{
    const auto next = (seed * 1'664'525U) + 1'013'904'223U;
    seed = next;
    return next;
}

void barrier() noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" ::: "memory");
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

struct Bench {
    const char* name;
    std::uint64_t ops;
    double ns_op;
};

void expect(const bool condition, const char* const message)
{
    if (!condition) {
        std::fprintf(stderr, "host benchmark correctness failed: %s\n", message);
        std::exit(1);
    }
}

template <typename Fn>
Bench measure(const char* const name, const std::uint64_t ops, Fn&& fn)
{
    const auto start = Clock::now();
    fn();
    const auto stop = Clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
    return Bench{
        .name = name,
        .ops = ops,
        .ns_op = ops == 0U ? 0.0 : static_cast<double>(ns) / static_cast<double>(ops),
    };
}

void print(const Bench& bench)
{
    std::printf("%-28s %12llu ops %10.2f ns/op\n",
                bench.name,
                static_cast<unsigned long long>(bench.ops),
                bench.ns_op);
}

void bench_spsc()
{
    constexpr std::uint32_t ops = 1'000'000U;
    arc::Spsc<std::uint32_t, 1024> queue;
    std::uint64_t sum{};

    const auto arc = measure("arc::Spsc", ops * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            expect(queue.try_push(i), "SPSC push during benchmark");
            std::uint32_t out{};
            expect(queue.try_pop(out), "SPSC pop during benchmark");
            sum += out;
        }
    });
    expect(sum == (static_cast<std::uint64_t>(ops - 1U) * ops) / 2U, "SPSC sum");
    sink += sum;
    print(arc);

    std::deque<std::uint32_t> baseline;
    sum = 0U;
    const auto standard = measure("std::deque", ops * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            baseline.push_back(i);
            const auto out = baseline.front();
            baseline.pop_front();
            sum += out;
        }
    });
    expect(sum == (static_cast<std::uint64_t>(ops - 1U) * ops) / 2U, "deque sum");
    sink += sum;
    print(standard);

    arc::Spsc<std::uint32_t, 1024> burst_queue;
    std::array<std::uint32_t, 1023> burst_in{};
    std::array<std::uint32_t, 1023> burst_out{};
    sum = 0U;
    const auto arc_burst = measure("arc::Spsc batch burst", ops * 2ULL, [&]() {
        for (std::uint32_t base = 0; base < ops; base += 1023U) {
            const auto batch = (ops - base) < 1023U ? (ops - base) : 1023U;
            for (std::uint32_t i = 0; i < batch; ++i) {
                burst_in[i] = base + i;
            }
            expect(burst_queue.push(std::span{burst_in}.first(batch)) == batch, "SPSC batch burst push");
            expect(burst_queue.pop(std::span{burst_out}.first(batch)) == batch, "SPSC batch burst pop");
            for (std::uint32_t i = 0; i < batch; ++i) {
                sum += burst_out[i];
            }
        }
    });
    expect(sum == (static_cast<std::uint64_t>(ops - 1U) * ops) / 2U, "SPSC burst sum");
    sink += sum;
    print(arc_burst);

    arc::Spsc<std::uint32_t, 2048> varied_queue;
    std::array<std::uint32_t, 1024> varied_in{};
    std::array<std::uint32_t, 1024> varied_out{};
    sum = 0U;
    std::uint64_t expected{};
    const auto arc_varied = measure("arc::Spsc batch varied", ops * 2ULL, [&]() {
        for (std::uint32_t sent = 0; sent < ops;) {
            const auto want = ((next_seed() >> 16U) & 255U) + 1U;
            const auto batch = (ops - sent) < want ? (ops - sent) : want;
            for (std::uint32_t i = 0; i < batch; ++i) {
                const auto value = sent + i;
                varied_in[i] = value;
                expected += value;
            }
            expect(varied_queue.push(std::span{varied_in}.first(batch)) == batch, "SPSC varied push");
            const auto first = batch / 2U;
            const auto second = batch - first;
            expect(varied_queue.pop(std::span{varied_out}.first(first)) == first, "SPSC varied first pop");
            for (std::uint32_t i = 0; i < first; ++i) {
                sum += varied_out[i];
            }
            expect(varied_queue.pop(std::span{varied_out}.first(second)) == second, "SPSC varied second pop");
            for (std::uint32_t i = 0; i < second; ++i) {
                sum += varied_out[i];
            }
            sent += batch;
        }
    });
    expect(sum == expected, "SPSC varied sum");
    sink += sum;
    print(arc_varied);

    arc::Spsc<std::uint32_t, 1024> single_burst_queue;
    sum = 0U;
    const auto arc_single_burst = measure("arc::Spsc single burst", ops * 2ULL, [&]() {
        for (std::uint32_t base = 0; base < ops; base += 1023U) {
            const auto batch = (ops - base) < 1023U ? (ops - base) : 1023U;
            for (std::uint32_t i = 0; i < batch; ++i) {
                expect(single_burst_queue.try_push(base + i), "SPSC single burst push");
            }
            for (std::uint32_t i = 0; i < batch; ++i) {
                std::uint32_t out{};
                expect(single_burst_queue.try_pop(out), "SPSC single burst pop");
                sum += out;
            }
        }
    });
    expect(sum == (static_cast<std::uint64_t>(ops - 1U) * ops) / 2U, "SPSC single burst sum");
    sink += sum;
    print(arc_single_burst);

    std::deque<std::uint32_t> burst_baseline;
    sum = 0U;
    const auto std_burst = measure("std::deque burst", ops * 2ULL, [&]() {
        for (std::uint32_t base = 0; base < ops; base += 1023U) {
            const auto batch = (ops - base) < 1023U ? (ops - base) : 1023U;
            for (std::uint32_t i = 0; i < batch; ++i) {
                burst_baseline.push_back(base + i);
            }
            for (std::uint32_t i = 0; i < batch; ++i) {
                const auto out = burst_baseline.front();
                burst_baseline.pop_front();
                sum += out;
            }
        }
    });
    expect(sum == (static_cast<std::uint64_t>(ops - 1U) * ops) / 2U, "deque burst sum");
    sink += sum;
    print(std_burst);
}

template <typename Queue>
Bench bench_mpsc_queue(const char* const name)
{
    constexpr std::uint32_t producers = 4U;
    constexpr std::uint32_t per_producer = 50'000U;
    constexpr std::uint32_t total = producers * per_producer;

    Queue queue;
    std::atomic<std::uint32_t> ready{};
    std::array<std::thread, producers> threads;
    std::uint64_t sum{};

    const auto result = measure(name, total, [&]() {
        for (std::uint32_t producer = 0; producer < producers; ++producer) {
            threads[producer] = std::thread([producer, producers, &queue, &ready]() {
                ready.fetch_add(1U, std::memory_order_release);
                while (ready.load(std::memory_order_acquire) != producers) {
                    std::this_thread::yield();
                }

                for (std::uint32_t i = 0; i < per_producer; ++i) {
                    const auto value = (producer << 24U) | i;
                    while (!queue.try_push(value)) {
                        std::this_thread::yield();
                    }
                }
            });
        }

        for (std::uint32_t seen = 0; seen < total;) {
            std::uint32_t value{};
            if (queue.try_pop(value)) {
                sum += value & 0x00ff'ffffU;
                ++seen;
            } else {
                std::this_thread::yield();
            }
        }

        for (auto& thread : threads) {
            thread.join();
        }
    });

    expect(sum == producers * ((static_cast<std::uint64_t>(per_producer - 1U) * per_producer) / 2U), name);
    sink += sum;
    return result;
}

void bench_mpsc()
{
    constexpr std::uint32_t producers = 4U;
    constexpr std::uint32_t per_producer = 50'000U;
    constexpr std::uint32_t total = producers * per_producer;

    print(bench_mpsc_queue<arc::Mpsc<std::uint32_t, 4096>>("arc::Mpsc 4p"));
    print(bench_mpsc_queue<arc::DenseMpsc<std::uint32_t, 4096>>("arc::DenseMpsc 4p"));

    std::deque<std::uint32_t> baseline;
    std::mutex lock;
    std::atomic<std::uint32_t> baseline_ready{};
    std::array<std::thread, producers> baseline_threads;
    std::uint64_t sum{};
    const auto standard = measure("mutex deque 4p", total, [&]() {
        for (std::uint32_t producer = 0; producer < producers; ++producer) {
            baseline_threads[producer] = std::thread([producer, producers, &baseline, &lock, &baseline_ready]() {
                baseline_ready.fetch_add(1U, std::memory_order_release);
                while (baseline_ready.load(std::memory_order_acquire) != producers) {
                    std::this_thread::yield();
                }

                for (std::uint32_t i = 0; i < per_producer; ++i) {
                    const auto value = (producer << 24U) | i;
                    const std::lock_guard guard(lock);
                    baseline.push_back(value);
                }
            });
        }

        for (std::uint32_t seen = 0; seen < total;) {
            std::uint32_t value{};
            bool got = false;
            {
                const std::lock_guard guard(lock);
                if (!baseline.empty()) {
                    value = baseline.front();
                    baseline.pop_front();
                    got = true;
                }
            }
            if (got) {
                sum += value & 0x00ff'ffffU;
                ++seen;
            } else {
                std::this_thread::yield();
            }
        }

        for (auto& thread : baseline_threads) {
            thread.join();
        }
    });
    expect(sum == producers * ((static_cast<std::uint64_t>(per_producer - 1U) * per_producer) / 2U), "mutex deque sum");
    sink += sum;
    print(standard);
}

void bench_fanin()
{
    constexpr std::uint32_t ops = 250'000U;
    arc::Fanin<std::uint32_t, 1024, 4> fan;
    std::uint64_t sum{};

    const auto arc = measure("arc::Fanin 4 lanes", ops * 8ULL, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            expect(fan.try_push<0>(i), "Fanin push 0");
            expect(fan.try_push<1>(i), "Fanin push 1");
            expect(fan.try_push<2>(i), "Fanin push 2");
            expect(fan.try_push<3>(i), "Fanin push 3");
            for (std::uint32_t lane = 0; lane < 4U; ++lane) {
                std::uint32_t out{};
                expect(fan.try_pop(out), "Fanin pop");
                sum += out;
            }
        }
    });
    expect(sum == 4ULL * ((static_cast<std::uint64_t>(ops - 1U) * ops) / 2U), "Fanin sum");
    sink += sum;
    print(arc);

    std::array<std::deque<std::uint32_t>, 4> queues{};
    sum = 0U;
    const auto standard = measure("std::deque fanin 4", ops * 8ULL, [&]() {
        std::size_t cursor{};
        for (std::uint32_t i = 0; i < ops; ++i) {
            for (auto& queue : queues) {
                queue.push_back(i);
            }
            for (std::uint32_t lane = 0; lane < 4U; ++lane) {
                for (std::size_t offset = 0; offset < queues.size(); ++offset) {
                    const auto index = (cursor + offset) % queues.size();
                    if (!queues[index].empty()) {
                        const auto out = queues[index].front();
                        queues[index].pop_front();
                        cursor = (index + 1U) % queues.size();
                        sum += out;
                        break;
                    }
                }
            }
        }
    });
    expect(sum == 4ULL * ((static_cast<std::uint64_t>(ops - 1U) * ops) / 2U), "deque fanin sum");
    sink += sum;
    print(standard);
}

void bench_seqreg()
{
    struct Snapshot {
        std::uint32_t a;
        std::uint32_t b;
        std::uint64_t c;
    };

    constexpr std::uint32_t ops = 500'000U;
    arc::SeqReg<Snapshot> reg;
    std::uint64_t sum{};

    const auto arc = measure("arc::SeqReg", ops * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            reg.write_unmasked(Snapshot{.a = i, .b = ~i, .c = static_cast<std::uint64_t>(i) * 3U});
            const auto out = reg.read();
            expect(out.b == ~out.a && out.c == static_cast<std::uint64_t>(out.a) * 3U, "SeqReg invariant");
            sum += out.a;
        }
    });
    sink += sum;
    print(arc);

    std::mutex lock;
    Snapshot snap{};
    sum = 0U;
    const auto standard = measure("mutex snapshot", ops * 2ULL, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            {
                const std::lock_guard guard(lock);
                snap = Snapshot{.a = i, .b = ~i, .c = static_cast<std::uint64_t>(i) * 3U};
            }
            Snapshot out{};
            {
                const std::lock_guard guard(lock);
                out = snap;
            }
            expect(out.b == ~out.a && out.c == static_cast<std::uint64_t>(out.a) * 3U, "mutex snapshot invariant");
            sum += out.a;
        }
    });
    sink += sum;
    print(standard);
}

void bench_memory()
{
    struct Payload {
        std::uint32_t a;
        std::uint32_t b;
        std::uint64_t c;
    };

    constexpr std::uint32_t ops = 1'000'000U;
    std::array<Payload, 1024> src{};
    std::array<Payload, 1024> dst{};
    for (std::size_t i = 0; i < src.size(); ++i) {
        src[i] = Payload{
            .a = static_cast<std::uint32_t>(i),
            .b = static_cast<std::uint32_t>(~i),
            .c = static_cast<std::uint64_t>(i) * 17U,
        };
    }

    std::uint64_t sum{};
    const auto span_copy = measure("std::copy span 16B", ops, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            const auto index = i & 1023U;
            std::copy_n(src.data() + index, 1U, dst.data() + ((index + 17U) & 1023U));
            sum += dst[(index + 17U) & 1023U].a;
        }
    });
    sink += sum;
    print(span_copy);

    sum = 0U;
    const auto memcpy_copy = measure("std::memcpy 16B", ops, [&]() {
        for (std::uint32_t i = 0; i < ops; ++i) {
            const auto index = i & 1023U;
            std::memcpy(dst.data() + ((index + 33U) & 1023U), src.data() + index, sizeof(Payload));
            sum += dst[(index + 33U) & 1023U].b;
        }
    });
    sink += sum;
    print(memcpy_copy);
}

void bench_dsp()
{
    constexpr std::size_t n = 1024U;
    constexpr std::uint32_t rounds = 200'000U;
    std::array<int, n> lhs{};
    std::array<int, n> rhs{};
    for (std::size_t i = 0; i < n; ++i) {
        lhs[i] = static_cast<int>((i % 17U) - 8U);
        rhs[i] = static_cast<int>((i % 11U) - 5U);
    }

    std::int64_t expected{};
    for (std::size_t i = 0; i < n; ++i) {
        expected += static_cast<std::int64_t>(lhs[i]) * rhs[i];
    }

    std::int64_t sum{};
    const auto arc = measure("arc::dsp::dot", rounds * n, [&]() {
        for (std::uint32_t round = 0; round < rounds; ++round) {
            sum += arc::dsp::dot(lhs.data(), rhs.data(), lhs.size());
        }
    });
    expect(sum == expected * rounds, "DSP dot sum");
    sink += static_cast<std::uint64_t>(sum);
    print(arc);
}

void bench_codecs()
{
    constexpr std::uint32_t rounds = 100'000U;
    std::array<std::uint8_t, 256> buffer{};
    std::array<std::uint8_t, 8> payload{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

    auto mqtt = measure("arc::Mqtt codec", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            const auto sample = next_seed();
            payload[0] = static_cast<std::uint8_t>(sample);
            barrier();
            const auto frame = arc::net::Mqtt::publish(
                buffer,
                "arc/bench",
                std::span(payload),
                static_cast<std::uint16_t>(sample));
            expect(frame.has_value(), "MQTT benchmark encode");
            const auto parsed = arc::net::Mqtt::parse(*frame);
            expect(parsed.has_value() && parsed->type == arc::net::MqttType::publish, "MQTT benchmark parse");
            const auto view = arc::net::Mqtt::view(*parsed);
            expect(view.has_value() && view->payload[0] == payload[0], "MQTT benchmark view");
            sink += view->packet;
        }
    });
    print(mqtt);

    auto ws = measure("arc::Ws codec", rounds, [&]() {
        std::array<std::uint8_t, 32> scratch{};
        for (std::uint32_t i = 0; i < rounds; ++i) {
            const auto sample = next_seed();
            payload[0] = static_cast<std::uint8_t>(sample);
            barrier();
            const auto frame = arc::net::Ws::binary(buffer, std::span(payload), true, sample);
            expect(frame.has_value(), "WS benchmark encode");
            const auto parsed = arc::net::Ws::parse(*frame, scratch);
            expect(parsed.has_value() && parsed->payload.size() == payload.size() && parsed->payload[0] == payload[0], "WS benchmark parse");
            sink += parsed->payload[0];
        }
    });
    print(ws);

    auto coap = measure("arc::Coap codec", rounds, [&]() {
        const std::array options{
            arc::net::Coap::text(
                static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
                "bench"),
        };
        for (std::uint32_t i = 0; i < rounds; ++i) {
            const auto sample = next_seed();
            payload[0] = static_cast<std::uint8_t>(sample);
            barrier();
            const auto frame = arc::net::Coap::message(
                buffer,
                arc::net::CoapType::confirmable,
                static_cast<std::uint8_t>(arc::net::CoapCode::post),
                static_cast<std::uint16_t>(sample),
                {},
                std::span(options),
                std::span(payload));
            expect(frame.has_value(), "CoAP benchmark encode");
            const auto parsed = arc::net::Coap::parse(*frame);
            expect(parsed.has_value() && parsed->payload.size() == payload.size() && parsed->payload[0] == payload[0], "CoAP benchmark parse");
            sink += parsed->payload[0];
        }
    });
    print(coap);
}

void bench_stream()
{
    struct SinkStream {
        std::array<std::uint8_t, 4096> data{};
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

    struct SourceStream {
        std::array<std::uint8_t, 4096> data{};
        std::size_t pos{};

        arc::Status send_all(const void*, std::size_t) noexcept
        {
            return arc::ok();
        }

        arc::Result<std::size_t> recv(void* const dst, const std::size_t bytes) noexcept
        {
            auto* out = static_cast<std::uint8_t*>(dst);
            const auto chunk = bytes > 7U ? 7U : bytes;
            for (std::size_t i = 0; i < chunk; ++i) {
                out[i] = data[(pos + i) % data.size()];
            }
            pos += chunk;
            return chunk;
        }
    };

    struct FrameSourceStream {
        std::size_t pos{};

        arc::Status send_all(const void*, std::size_t) noexcept
        {
            return arc::ok();
        }

        arc::Result<std::size_t> recv(void* const dst, const std::size_t bytes) noexcept
        {
            auto* out = static_cast<std::uint8_t*>(dst);
            const auto chunk = bytes > 7U ? 7U : bytes;
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

    constexpr std::uint32_t rounds = 500'000U;
    std::array<std::uint8_t, 32> payload{};
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<std::uint8_t>(i);
    }

    SinkStream stream;
    const auto raw = measure("arc::Stream write", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(next_seed());
            barrier();
            expect(arc::net::Stream::write(stream, std::span(payload)).has_value(), "Stream write");
        }
    });
    sink += stream.pos + stream.checksum;
    print(raw);

    const auto framed = measure("arc::Stream frame16", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            payload[0] = static_cast<std::uint8_t>(next_seed());
            barrier();
            expect(arc::net::Stream::write_frame16(stream, std::span(payload)).has_value(), "Stream frame16");
        }
    });
    sink += stream.pos + stream.checksum;
    print(framed);

    SourceStream source;
    for (std::size_t i = 0; i < source.data.size(); ++i) {
        source.data[i] = static_cast<std::uint8_t>(i);
    }
    const auto read = measure("arc::Stream read_exact", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            std::array<std::uint8_t, 32> out{};
            barrier();
            expect(arc::net::Stream::read_exact(source, std::span(out)).has_value(), "Stream read_exact");
            sink += out[i & 31U];
        }
    });
    print(read);

    FrameSourceStream frame_source;
    const auto read_frame = measure("arc::Stream read_frame16", rounds, [&]() {
        for (std::uint32_t i = 0; i < rounds; ++i) {
            std::array<std::uint8_t, 32> out{};
            barrier();
            const auto got = arc::net::Stream::read_frame16(frame_source, std::span(out));
            expect(got.has_value() && *got == out.size(), "Stream read_frame16");
            sink += out[i & 31U];
        }
    });
    print(read_frame);
}

}  // namespace

int main()
{
    std::puts("arc host benchmark: correctness + throughput");
    bench_spsc();
    bench_mpsc();
    bench_fanin();
    bench_seqreg();
    bench_memory();
    bench_stream();
    bench_dsp();
    bench_codecs();
    std::printf("arc host benchmark: OK (%llu)\n", static_cast<unsigned long long>(sink));
}
