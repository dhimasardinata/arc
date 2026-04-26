#include <array>
#include <atomic>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <utility>

#include "arc/dsp.hpp"
#include "arc/fanin.hpp"
#include "arc/file.hpp"
#include "arc/init.hpp"
#include "arc/coap.hpp"
#include "arc/mqtt.hpp"
#include "arc/mpsc.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/ws.hpp"

namespace {

static_assert(sizeof(arc::InitTxn) == sizeof(void*));
static_assert(sizeof(arc::RefLease) == sizeof(void*));

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define ARC_HOST_TSAN 1
#endif
#endif

#if defined(__SANITIZE_THREAD__)
#define ARC_HOST_TSAN 1
#endif

void expect(const bool condition, const char* const message)
{
    if (!condition) {
        std::fprintf(stderr, "host test failed: %s\n", message);
        std::exit(1);
    }
}

template <typename Fn>
void expect_abort(Fn&& fn, const char* const message)
{
    const auto pid = fork();
    expect(pid >= 0, "fork");

    if (pid == 0) {
        fn();
        _exit(0);
    }

    int status = 0;
    expect(waitpid(pid, &status, 0) == pid, "waitpid");
    expect(WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) != 0), message);
}

void test_spsc()
{
    arc::Spsc<int, 4> queue;
    expect(queue.cap() == 3U, "SPSC usable capacity");
    expect(queue.bytes() == sizeof(queue), "SPSC bytes");
    expect(queue.empty(), "SPSC starts empty");
    expect(queue.try_push(1), "SPSC push 1");
    expect(queue.try_push(2), "SPSC push 2");
    expect(queue.try_push(3), "SPSC push 3");
    expect(!queue.try_push(4), "SPSC full rejects");

    int value{};
    expect(queue.try_pop(value) && value == 1, "SPSC pop 1");
    expect(queue.try_pop(value) && value == 2, "SPSC pop 2");
    expect(queue.try_push(4), "SPSC wrap push");
    expect(queue.try_pop(value) && value == 3, "SPSC pop 3");
    expect(queue.try_pop(value) && value == 4, "SPSC pop 4");
    expect(!queue.try_pop(value), "SPSC empty rejects");
}

void test_checked_spsc()
{
    arc::Audit<arc::Spsc<int, 4>> queue;
    expect(queue.cap() == 3U, "Audit SPSC usable capacity");
    expect(queue.try_push(1), "Audit SPSC push 1");

    int value{};
    expect(queue.try_pop(value) && value == 1, "Audit SPSC pop 1");

    expect_abort(
        []() {
            arc::Audit<arc::Spsc<int, 4>> lane;
            expect(lane.try_push(1), "Audit SPSC child first push");
            std::thread other([&lane]() {
                static_cast<void>(lane.try_push(2));
            });
            other.join();
        },
        "Audit SPSC rejects a second producer");
}

void test_mpsc_single()
{
    arc::Mpsc<int, 4> queue;
    expect(queue.cap() == 4U, "MPSC capacity");
    expect(queue.cell_align() == arc::cache_line, "MPSC cache-line alignment");
    expect(queue.bytes() == sizeof(queue), "MPSC bytes");
    expect(queue.try_push(7), "MPSC push 7");
    expect(queue.try_push(8), "MPSC push 8");
    expect(queue.try_push(9), "MPSC push 9");
    expect(queue.try_push(10), "MPSC push 10");
    expect(!queue.try_push(11), "MPSC full rejects");

    int value{};
    expect(queue.try_pop(value) && value == 7, "MPSC pop 7");
    expect(queue.try_pop(value) && value == 8, "MPSC pop 8");
    expect(queue.try_push(11), "MPSC wrap push");
    expect(queue.try_pop(value) && value == 9, "MPSC pop 9");
    expect(queue.try_pop(value) && value == 10, "MPSC pop 10");
    expect(queue.try_pop(value) && value == 11, "MPSC pop 11");
    expect(!queue.try_pop(value), "MPSC empty rejects");
}

void test_compact_mpsc()
{
    arc::DenseMpsc<std::uint32_t, 1024> queue;
    expect(queue.cap() == 1024U, "Dense MPSC capacity");
    expect(queue.cell_align() == alignof(std::uint32_t), "Dense MPSC alignment");
    expect(queue.cell_bytes() < arc::Mpsc<std::uint32_t, 1024>::cell_bytes(), "Dense MPSC cell shrinks");
    expect(queue.bytes() < arc::Mpsc<std::uint32_t, 1024>::bytes(), "Dense MPSC bytes shrink");
    expect(queue.try_push(7U), "Dense MPSC push");

    std::uint32_t value{};
    expect(queue.try_pop(value) && value == 7U, "Dense MPSC pop");
}

void test_checked_mpsc()
{
    arc::Audit<arc::Mpsc<int, 4>> queue;
    expect(queue.try_push(1), "Audit MPSC push 1");

    int value{};
    expect(queue.try_pop(value) && value == 1, "Audit MPSC pop 1");

    expect_abort(
        []() {
            arc::Audit<arc::Mpsc<int, 4>> lane;
            expect(lane.try_push(1), "Audit MPSC child push");
            int first{};
            expect(lane.try_pop(first) && first == 1, "Audit MPSC child first pop");
            expect(lane.try_push(2), "Audit MPSC child second push");
            std::thread other([&lane]() {
                int second{};
                static_cast<void>(lane.try_pop(second));
            });
            other.join();
        },
        "Audit MPSC rejects a second consumer");
}

void test_mpsc_threads()
{
    constexpr std::uint32_t producers = 4U;
    constexpr std::uint32_t per_producer = 2048U;
    constexpr std::uint32_t total = producers * per_producer;

    arc::Mpsc<std::uint32_t, 1024> queue;
    std::array<std::uint32_t, producers> counts{};
    std::array<std::thread, producers> threads;
    std::atomic<std::uint32_t> ready{};

    for (std::uint32_t producer = 0; producer < producers; ++producer) {
        threads[producer] = std::thread([producer, &queue, &ready]() {
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

    std::uint32_t seen{};
    while (seen < total) {
        std::uint32_t value{};
        if (queue.try_pop(value)) {
            const auto producer = value >> 24U;
            expect(producer < producers, "MPSC producer tag");
            ++counts[producer];
            ++seen;
        } else {
            std::this_thread::yield();
        }
    }

    for (auto& thread : threads) {
        thread.join();
    }
    for (const auto count : counts) {
        expect(count == per_producer, "MPSC threaded producer count");
    }
    expect(queue.empty(), "MPSC threaded drain empty");
}

void test_fanin()
{
    arc::Fanin<int, 4, 3> fan;
    expect(fan.producers() == 3U, "Fanin producer count");
    expect(fan.cap() == 3U, "Fanin lane capacity");
    expect(fan.bytes() == sizeof(fan), "Fanin bytes");
    expect(fan.try_push<0>(10), "Fanin push lane 0");
    expect(fan.try_push<1>(20), "Fanin push lane 1");
    expect(fan.try_push<2>(30), "Fanin push lane 2");
    expect(fan.try_push<0>(11), "Fanin second lane 0");

    std::size_t producer{};
    int value{};
    expect(fan.try_pop(producer, value) && producer == 0U && value == 10, "Fanin pop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Fanin pop lane 1");
    expect(fan.try_pop(producer, value) && producer == 2U && value == 30, "Fanin pop lane 2");
    expect(fan.try_pop(producer, value) && producer == 0U && value == 11, "Fanin round-robin resume");
    expect(!fan.try_pop(producer, value), "Fanin empty rejects");
}

void test_checked_fanin()
{
    arc::Audit<arc::Fanin<int, 4, 2>> fan;
    expect(fan.producers() == 2U, "Audit Fanin producer count");
    expect(fan.try_push<0>(10), "Audit Fanin push lane 0");
    expect(fan.try_push<1>(20), "Audit Fanin push lane 1");

    std::size_t producer{};
    int value{};
    expect(fan.try_pop(producer, value) && producer == 0U && value == 10, "Audit Fanin pop lane 0");
    expect(fan.try_pop(producer, value) && producer == 1U && value == 20, "Audit Fanin pop lane 1");
}

void test_mqtt()
{
    std::array<std::uint8_t, 256> buffer{};

    const auto connect = arc::net::Mqtt::connect(
        buffer,
        {
            .client = "arc-host",
            .user = "arc",
            .pass = "secret",
            .keep_alive = 30U,
            .clean = true,
        });
    expect(connect.has_value(), "MQTT connect encodes");
    expect((*connect)[0] == 0x10U, "MQTT connect type");
    expect((*connect)[2] == 0x00U && (*connect)[3] == 0x04U, "MQTT protocol length");
    expect((*connect)[4] == 'M' && (*connect)[7] == 'T', "MQTT protocol name");

    const std::array<std::uint8_t, 5> payload{1U, 2U, 3U, 4U, 5U};
    const auto publish = arc::net::Mqtt::publish(
        buffer,
        "arc/topic",
        std::span(payload),
        7U,
        arc::net::MqttQos::at_least_once,
        false,
        true);
    expect(publish.has_value(), "MQTT publish encodes");

    const auto packet = arc::net::Mqtt::parse(*publish);
    expect(packet.has_value(), "MQTT publish parses");
    expect(packet->type == arc::net::MqttType::publish, "MQTT publish type");
    expect(packet->retain(), "MQTT publish retain");
    expect(packet->qos() == arc::net::MqttQos::at_least_once, "MQTT publish qos");

    const auto view = arc::net::Mqtt::view(*packet);
    expect(view.has_value(), "MQTT publish view");
    expect(view->packet == 7U, "MQTT publish packet id");
    expect(view->topic.size() == 9U, "MQTT publish topic bytes");
    expect(std::memcmp(view->topic.data(), "arc/topic", view->topic.size()) == 0, "MQTT publish topic");
    expect(view->payload.size() == payload.size(), "MQTT publish payload size");
    expect(std::memcmp(view->payload.data(), payload.data(), payload.size()) == 0, "MQTT publish payload");

    const std::array subscriptions{
        arc::net::MqttSubscription{.filter = "arc/+/status", .qos = arc::net::MqttQos::at_least_once},
        arc::net::MqttSubscription{.filter = "arc/#", .qos = arc::net::MqttQos::at_most_once},
    };
    const auto subscribe = arc::net::Mqtt::subscribe(buffer, 9U, std::span(subscriptions));
    expect(subscribe.has_value(), "MQTT subscribe encodes");
    expect((*subscribe)[0] == 0x82U, "MQTT subscribe flags");
    expect((*subscribe)[2] == 0x00U && (*subscribe)[3] == 0x09U, "MQTT subscribe packet id");

    const std::array<std::uint8_t, 4> connack_raw{0x20U, 0x02U, 0x01U, 0x00U};
    const auto connack_packet = arc::net::Mqtt::parse(connack_raw);
    expect(connack_packet.has_value(), "MQTT connack parses");
    const auto connack = arc::net::Mqtt::connack(*connack_packet);
    expect(connack.has_value(), "MQTT connack view");
    expect(connack->session, "MQTT connack session");
    expect(connack->code == 0U, "MQTT connack code");

    const std::array<std::uint8_t, 5> suback_raw{0x90U, 0x03U, 0x00U, 0x09U, 0x01U};
    const auto suback_packet = arc::net::Mqtt::parse(suback_raw);
    expect(suback_packet.has_value(), "MQTT suback parses");
    const auto suback = arc::net::Mqtt::suback(*suback_packet);
    expect(suback.has_value(), "MQTT suback view");
    expect(suback->packet == 9U, "MQTT suback packet id");
    expect(suback->codes.size() == 1U && suback->codes[0] == 0x01U, "MQTT suback code");

    expect(arc::net::Mqtt::match("arc/+/status", "arc/node/status"), "MQTT single wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc/node/status"), "MQTT multi wildcard");
    expect(arc::net::Mqtt::match("arc/#", "arc"), "MQTT parent wildcard root");
    expect(!arc::net::Mqtt::match("arc#", "arc"), "MQTT invalid hash placement");
    expect(!arc::net::Mqtt::match("arc+status", "arc/status"), "MQTT invalid plus placement");
    expect(!arc::net::Mqtt::match("arc/+/status", "arc/node/state"), "MQTT mismatch");
    expect(!arc::net::Mqtt::match("arc/#", "$SYS/broker"), "MQTT system topic isolation");
}

void test_ws()
{
    std::array<char, 32> accept_buf{};
    const auto accept = arc::net::Ws::accept(accept_buf, "dGhlIHNhbXBsZSBub25jZQ==");
    expect(accept.has_value(), "WS accept encodes");
    expect(
        std::memcmp(accept->data(), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept->size()) == 0,
        "WS accept value");

    const std::array<std::uint8_t, 16> nonce{
        0x01U,
        0x23U,
        0x45U,
        0x67U,
        0x89U,
        0xabU,
        0xcdU,
        0xefU,
        0x10U,
        0x32U,
        0x54U,
        0x76U,
        0x98U,
        0xbaU,
        0xdcU,
        0xfeU,
    };
    std::array<char, 32> key_buf{};
    const auto key = arc::net::Ws::key(key_buf, nonce);
    expect(key.has_value() && key->size() == 24U, "WS key bytes");

    std::array<std::uint8_t, 128> frame{};
    const auto text = arc::net::Ws::text(frame, "arc", true, 0x11223344U);
    expect(text.has_value(), "WS text encodes");

    std::array<std::uint8_t, 32> scratch{};
    const auto parsed = arc::net::Ws::parse(*text, scratch);
    expect(parsed.has_value(), "WS text parses");
    expect(parsed->opcode == arc::net::WsOpcode::text, "WS text opcode");
    expect(parsed->masked, "WS text masked");
    expect(parsed->fin, "WS text fin");
    expect(parsed->payload.size() == 3U, "WS text payload size");
    expect(std::memcmp(parsed->payload.data(), "arc", parsed->payload.size()) == 0, "WS text payload");

    const std::array<std::uint8_t, 3> ping_data{1U, 2U, 3U};
    const auto ping = arc::net::Ws::ping(frame, std::span(ping_data));
    expect(ping.has_value(), "WS ping encodes");
    const auto ping_frame = arc::net::Ws::parse(*ping);
    expect(ping_frame.has_value(), "WS ping parses");
    expect(ping_frame->opcode == arc::net::WsOpcode::ping, "WS ping opcode");

    const std::array<std::uint8_t, 3> reason{'b', 'y', 'e'};
    const auto close = arc::net::Ws::close(frame, 1000U, std::span(reason));
    expect(close.has_value(), "WS close encodes");
    const auto close_frame = arc::net::Ws::parse(*close);
    expect(close_frame.has_value(), "WS close parses");
    const auto close_view = arc::net::Ws::close_view(*close_frame);
    expect(close_view.has_value(), "WS close view");
    expect(close_view->code == 1000U, "WS close code");
    expect(close_view->reason.size() == reason.size(), "WS close reason size");
    expect(std::memcmp(close_view->reason.data(), reason.data(), reason.size()) == 0, "WS close reason");
}

void test_coap()
{
    std::array<std::uint8_t, 256> buffer{};
    const std::array<std::uint8_t, 2> token{0xdeU, 0xadU};
    const std::array options{
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            "sensors"),
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path),
            "temp"),
        arc::net::Coap::text(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_query),
            "units=c"),
    };

    const auto request = arc::net::Coap::message(
        buffer,
        arc::net::CoapType::confirmable,
        static_cast<std::uint8_t>(arc::net::CoapCode::get),
        0x1234U,
        std::span(token),
        std::span(options));
    expect(request.has_value(), "CoAP request encodes");

    const auto parsed = arc::net::Coap::parse(*request);
    expect(parsed.has_value(), "CoAP request parses");
    expect(parsed->type == arc::net::CoapType::confirmable, "CoAP request type");
    expect(parsed->code == static_cast<std::uint8_t>(arc::net::CoapCode::get), "CoAP request code");
    expect(parsed->id == 0x1234U, "CoAP request id");
    expect(parsed->token.size() == token.size(), "CoAP token size");
    expect(std::memcmp(parsed->token.data(), token.data(), token.size()) == 0, "CoAP token");

    std::size_t offset = 0U;
    std::uint16_t number = 0U;
    const auto first = arc::net::Coap::next(parsed->options, offset, number);
    expect(first.has_value(), "CoAP option 1");
    expect(first->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "CoAP option 1 number");
    expect(std::memcmp(first->value.data(), "sensors", first->value.size()) == 0, "CoAP option 1 value");

    const auto second = arc::net::Coap::next(parsed->options, offset, number);
    expect(second.has_value(), "CoAP option 2");
    expect(second->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_path), "CoAP option 2 number");
    expect(std::memcmp(second->value.data(), "temp", second->value.size()) == 0, "CoAP option 2 value");

    const auto third = arc::net::Coap::next(parsed->options, offset, number);
    expect(third.has_value(), "CoAP option 3");
    expect(third->number == static_cast<std::uint16_t>(arc::net::CoapOptionNumber::uri_query), "CoAP option 3 number");
    expect(std::memcmp(third->value.data(), "units=c", third->value.size()) == 0, "CoAP option 3 value");

    const auto none = arc::net::Coap::next(parsed->options, offset, number);
    expect(!none.has_value() && none.error() == ESP_ERR_NOT_FOUND, "CoAP option end");

    const std::array<std::uint8_t, 1> format{50U};
    const std::array response_options{
        arc::net::Coap::option(
            static_cast<std::uint16_t>(arc::net::CoapOptionNumber::content_format),
            std::span(format)),
    };
    const std::array<std::uint8_t, 4> body{'4', '2', '.', '0'};
    const auto response = arc::net::Coap::message(
        buffer,
        arc::net::CoapType::acknowledgement,
        static_cast<std::uint8_t>(arc::net::CoapCode::content),
        0x1234U,
        std::span(token),
        std::span(response_options),
        std::span(body));
    expect(response.has_value(), "CoAP response encodes");

    const auto parsed_response = arc::net::Coap::parse(*response);
    expect(parsed_response.has_value(), "CoAP response parses");
    expect(parsed_response->type == arc::net::CoapType::acknowledgement, "CoAP response type");
    expect(parsed_response->code == static_cast<std::uint8_t>(arc::net::CoapCode::content), "CoAP response code");
    expect(parsed_response->payload.size() == body.size(), "CoAP payload size");
    expect(std::memcmp(parsed_response->payload.data(), body.data(), body.size()) == 0, "CoAP payload");

    const auto ping = arc::net::Coap::ping(buffer, 0x40U);
    expect(ping.has_value(), "CoAP ping encodes");
    const auto ping_msg = arc::net::Coap::parse(*ping);
    expect(ping_msg.has_value(), "CoAP ping parses");
    expect(ping_msg->type == arc::net::CoapType::confirmable, "CoAP ping type");
    expect(ping_msg->code == static_cast<std::uint8_t>(arc::net::CoapCode::empty), "CoAP ping code");

    const auto reset = arc::net::Coap::reset(buffer, 0x40U);
    expect(reset.has_value(), "CoAP reset encodes");
}

void test_dsp()
{
    const std::array<int, 4> lhs{1, 2, 3, 4};
    const std::array<int, 4> rhs{5, 6, 7, 8};
    expect(arc::dsp::dot(lhs.data(), rhs.data(), lhs.size()) == 70, "DSP dot");

    std::array<int, 4> out{};
    arc::dsp::scale(out.data(), lhs.data(), 3, lhs.size());
    expect((out == std::array<int, 4>{3, 6, 9, 12}), "DSP scale");
    arc::dsp::mix(out.data(), lhs.data(), rhs.data(), lhs.size());
    expect((out == std::array<int, 4>{6, 8, 10, 12}), "DSP mix");
    arc::dsp::mac(out.data(), lhs.data(), 2, lhs.size());
    expect((out == std::array<int, 4>{8, 12, 16, 20}), "DSP mac");

    const std::array<int, 5> wave{-2, 4, -7, 1, 3};
    expect(arc::dsp::peak(wave.data(), wave.size()) == 7, "DSP peak");

    using Filter = arc::dsp::Fir<int, 3>;
    Filter::State state{};
    const Filter::Coeffs taps{1, 2, 3};
    const std::array<int, 4> input{1, 2, 3, 4};
    std::array<int, 4> filtered{};
    Filter::run(filtered.data(), state, taps, input.data(), input.size());
    expect((filtered == std::array<int, 4>{1, 4, 10, 16}), "DSP FIR");
    Filter::clear(state);
    expect(Filter::step(state, taps, 5) == 5, "DSP FIR clear");
}

void test_seqreg()
{
    struct Snapshot {
        std::uint32_t seq;
        std::uint32_t inv;
        std::uint64_t total;
    };

    arc::SeqReg<Snapshot> reg;
    reg.write_unmasked(Snapshot{
        .seq = 0U,
        .inv = ~0U,
        .total = 0U,
    });
    std::atomic<bool> running{true};

    std::thread writer([&]() {
        for (std::uint32_t seq = 1; seq <= 20000U; ++seq) {
            reg.write_unmasked(Snapshot{
                .seq = seq,
                .inv = ~seq,
                .total = static_cast<std::uint64_t>(seq) * 3U,
            });
        }
        running.store(false, std::memory_order_release);
    });

    std::uint32_t last{};
    Snapshot snap{};
    while (running.load(std::memory_order_acquire)) {
        if (!reg.try_read(snap)) {
            std::this_thread::yield();
            continue;
        }

        expect(snap.inv == ~snap.seq, "SeqReg snapshot inverse");
        expect(snap.total == static_cast<std::uint64_t>(snap.seq) * 3U, "SeqReg snapshot total");
        expect(snap.seq >= last, "SeqReg monotonic snapshot");
        last = snap.seq;
    }

    writer.join();

    const auto final = reg.read();
    expect(final.seq == 20000U, "SeqReg final sequence");
    expect(final.inv == ~20000U, "SeqReg final inverse");
    expect(final.total == static_cast<std::uint64_t>(20000U) * 3U, "SeqReg final total");
}

void test_file()
{
    char path[] = "/tmp/arc-host-file-XXXXXX";
    const auto fd = mkstemp(path);
    expect(fd >= 0, "File temp path");
    expect(close(fd) == 0, "File temp close");

    {
        auto file = arc::File::open(path, "wb");
        expect(file.has_value(), "File open write");

        auto handle = std::move(file).value();
        const std::array<std::uint8_t, 4> payload{1U, 3U, 3U, 7U};
        const auto wrote = handle.write(payload.data(), payload.size());
        expect(wrote.has_value() && wrote.value() == payload.size(), "File write bytes");
        expect(handle.flush() == ESP_OK, "File flush");
        expect(handle.close() == ESP_OK, "File close write");
    }

    {
        auto file = arc::File::open(path, "rb");
        expect(file.has_value(), "File open read");

        auto handle = std::move(file).value();
        std::array<std::uint8_t, 4> payload{};
        const auto read = handle.read(payload.data(), payload.size());
        expect(read.has_value() && read.value() == payload.size(), "File read bytes");
        expect((payload == std::array<std::uint8_t, 4>{1U, 3U, 3U, 7U}), "File payload");
        expect(handle.close() == ESP_OK, "File close read");
    }

    {
        auto missing = arc::File::open("/tmp/arc-host-file-missing", "rb");
        expect(!missing.has_value(), "File missing path");
        expect(missing.error() == ESP_ERR_NOT_FOUND, "File missing error");
    }

    expect(std::remove(path) == 0, "File cleanup");
}

void test_refinit()
{
    std::uint32_t state{};
    {
        auto txn = arc::InitTxn::begin(state);
        expect(static_cast<bool>(txn), "InitTxn first begin initializes");
    }
    expect(arc::InitTxn::begin(state).pass(), "InitTxn destructor rolls back busy state");
    expect(!arc::InitTxn::begin(state), "InitTxn ready state does not initialize");
    expect(arc::Init::take(state), "InitTxn pass leaves ready state");
    arc::Init::fail(state);

    auto txn = arc::InitTxn::begin(state);
    expect(static_cast<bool>(txn), "InitTxn move source active");
    auto moved = std::move(txn);
    expect(!static_cast<bool>(txn), "InitTxn move clears source");
    expect(static_cast<bool>(moved), "InitTxn move keeps destination");
    expect(moved.fail(), "InitTxn explicit fail");

    expect(arc::RefInit::begin(state), "RefInit first begin initializes");
    arc::RefInit::pass(state);
    expect(arc::RefInit::refs(state) == 1U, "RefInit initializer owns first ref");
    expect(!arc::RefInit::begin(state), "RefInit second begin shares ready resource");
    expect(arc::RefInit::refs(state) == 2U, "RefInit shared begin increments refs");
    expect(arc::RefInit::take(state), "RefInit take existing resource");
    expect(arc::RefInit::refs(state) == 3U, "RefInit take increments refs");
    expect(!arc::RefInit::drop(state), "RefInit non-last drop keeps resource live");
    expect(!arc::RefInit::drop(state), "RefInit second non-last drop keeps resource live");
    expect(arc::RefInit::drop(state), "RefInit last drop owns teardown");
    arc::RefInit::fail(state);
    expect(arc::RefInit::refs(state) == 0U, "RefInit teardown returns empty");
    expect(!arc::RefInit::take(state), "RefInit take empty resource fails");

    expect(arc::RefInit::begin(state), "RefLease init begin");
    arc::RefInit::pass(state);
    {
        auto lease = arc::RefLease::take(state);
        expect(static_cast<bool>(lease), "RefLease takes ready resource");
        expect(arc::RefInit::refs(state) == 2U, "RefLease increments refs");

        auto moved = std::move(lease);
        expect(!static_cast<bool>(lease), "RefLease move clears source");
        expect(static_cast<bool>(moved), "RefLease move keeps destination");
    }
    expect(arc::RefInit::refs(state) == 1U, "RefLease destructor drops non-final ref");

    auto lease = arc::RefLease::take(state);
    expect(!lease.release(), "RefLease explicit non-final release");
    expect(arc::RefInit::refs(state) == 1U, "RefLease release leaves owner ref");

    auto owner = arc::RefLease::adopt(state);
    expect(owner.release(), "RefLease explicit final release owns teardown");
    arc::RefInit::fail(state);
    expect(arc::RefInit::refs(state) == 0U, "RefLease final teardown returns empty");

    expect_abort(
        []() {
            std::uint32_t local{};
            expect(arc::RefInit::begin(local), "RefLease abort begin");
            arc::RefInit::pass(local);
            auto final = arc::RefLease::adopt(local);
            static_cast<void>(final);
        },
        "RefLease final destructor aborts");
}

}  // namespace

int main()
{
    test_spsc();
    test_checked_spsc();
    test_mpsc_single();
    test_compact_mpsc();
    test_checked_mpsc();
    test_mpsc_threads();
    test_fanin();
    test_checked_fanin();
    test_mqtt();
    test_ws();
    test_coap();
    test_dsp();
    test_seqreg();
    test_file();
    test_refinit();
    std::puts("arc host tests: OK");
}
