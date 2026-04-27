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
#include "arc/mpsc.hpp"
#include "arc/mqtt.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/ws.hpp"

namespace {

using Clock = std::chrono::steady_clock;

volatile std::uint64_t sink{};
volatile std::uint32_t seed{0x1234'5678U};

[[gnu::noinline]] std::uint32_t next_seed() noexcept
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
    std::printf("%-24s %12llu ops %10.2f ns/op\n",
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
}

void bench_mpsc()
{
    constexpr std::uint32_t producers = 4U;
    constexpr std::uint32_t per_producer = 50'000U;
    constexpr std::uint32_t total = producers * per_producer;

    arc::Mpsc<std::uint32_t, 4096> queue;
    std::atomic<std::uint32_t> ready{};
    std::array<std::thread, producers> threads;
    std::uint64_t sum{};

    const auto arc = measure("arc::Mpsc 4p", total, [&]() {
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
    expect(sum == producers * ((static_cast<std::uint64_t>(per_producer - 1U) * per_producer) / 2U), "MPSC sum");
    sink += sum;
    print(arc);

    std::deque<std::uint32_t> baseline;
    std::mutex lock;
    std::atomic<std::uint32_t> baseline_ready{};
    std::array<std::thread, producers> baseline_threads;
    sum = 0U;
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

}  // namespace

int main()
{
    std::puts("arc host benchmark: correctness + throughput");
    bench_spsc();
    bench_mpsc();
    bench_seqreg();
    bench_dsp();
    bench_codecs();
    std::printf("arc host benchmark: OK (%llu)\n", static_cast<unsigned long long>(sink));
}
