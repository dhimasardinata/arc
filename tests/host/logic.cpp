#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <utility>

#include "arc/any.hpp"
#include "arc/caps.hpp"
#include "arc/concepts.hpp"
#include "arc/dsp.hpp"
#include "arc/fanin.hpp"
#include "arc/file.hpp"
#include "arc/init.hpp"
#include "arc/coap.hpp"
#include "arc/mqtt.hpp"
#include "arc/mpsc.hpp"
#include "arc/seq.hpp"
#include "arc/spsc.hpp"
#include "arc/stream.hpp"
#include "arc/ws.hpp"

namespace {

static_assert(sizeof(arc::InitTxn) == sizeof(void*));
static_assert(sizeof(arc::RefInitTxn) == sizeof(void*));
static_assert(sizeof(arc::RefLease) == sizeof(void*));
static_assert(sizeof(arc::TryGate) == sizeof(void*));
static_assert(sizeof(arc::AnyOut) == 6U * sizeof(void*));
static_assert(sizeof(arc::AnyIn) == 3U * sizeof(void*));
static_assert(sizeof(arc::AnyI2c) == 4U * sizeof(void*));
static_assert(sizeof(arc::AnySpi) == 4U * sizeof(void*));
static_assert(sizeof(arc::AnyUart) == 6U * sizeof(void*));

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

struct MockStaticI2c {
    static inline std::size_t sent{};
    static inline std::size_t received{};
    static inline std::size_t xfer_tx{};
    static inline std::size_t xfer_rx{};
    static inline int timeout{};

    [[nodiscard]] static esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        sent = data == nullptr ? 0U : bytes;
        timeout = timeout_ms;
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const int timeout_ms) noexcept
    {
        received = data == nullptr ? 0U : bytes;
        timeout = timeout_ms;
        std::memset(data, 0x5A, bytes);
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t xfer(
        const void* const tx,
        const std::size_t tx_bytes,
        void* const rx,
        const std::size_t rx_bytes,
        const int timeout_ms) noexcept
    {
        xfer_tx = tx == nullptr ? 0U : tx_bytes;
        xfer_rx = rx == nullptr ? 0U : rx_bytes;
        timeout = timeout_ms;
        const auto* in = static_cast<const std::uint8_t*>(tx);
        auto* out = static_cast<std::uint8_t*>(rx);
        for (std::size_t i = 0; i < tx_bytes && i < rx_bytes; ++i) {
            out[i] = static_cast<std::uint8_t>(in[i] ^ 0xFFU);
        }
        return ESP_OK;
    }
};

static_assert(arc::I2cDevice<MockStaticI2c>);

struct MockStaticOut {
    static inline bool level{};
    static inline std::uint32_t configured{};
    static inline std::uint32_t writes{};

    static void out() noexcept
    {
        ++configured;
    }

    static void hi() noexcept
    {
        level = true;
        ++writes;
    }

    static void lo() noexcept
    {
        level = false;
        ++writes;
    }

    static void toggle() noexcept
    {
        level = !level;
        ++writes;
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }
};

static_assert(arc::DigitalOut<MockStaticOut>);

struct MockInput {
    bool level{};
    std::uint32_t configured{};

    arc::Status in() noexcept
    {
        ++configured;
        return arc::ok();
    }

    [[nodiscard]] bool high() const noexcept
    {
        return level;
    }
};

struct MockStaticInput {
    static inline bool level{};

    static esp_err_t in() noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static bool high() noexcept
    {
        return level;
    }
};

static_assert(arc::DigitalIn<MockStaticInput>);

struct MockSpi {
    std::size_t sent{};
    std::size_t received{};
    std::size_t xfer_bytes{};
    std::uint32_t hz{};
    std::uint32_t flags{};

    [[nodiscard]] esp_err_t send(
        const void* const data,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        sent = data == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t recv(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        received = data == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        std::memset(data, 0xC3, bytes);
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t xfer(
        const void* const tx,
        void* const rx,
        const std::size_t bytes,
        const std::uint32_t speed,
        const std::uint32_t mode_flags) noexcept
    {
        xfer_bytes = tx == nullptr || rx == nullptr ? 0U : bytes;
        hz = speed;
        flags = mode_flags;
        std::memcpy(rx, tx, bytes);
        return ESP_OK;
    }
};

struct MockStaticSpi {
    [[nodiscard]] static esp_err_t send(
        const void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t recv(
        void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t xfer(
        const void*,
        void*,
        std::size_t,
        std::uint32_t,
        std::uint32_t) noexcept
    {
        return ESP_OK;
    }
};

static_assert(arc::SpiDevice<MockStaticSpi>);

struct MockUart {
    std::array<std::uint8_t, 8> rx{0xA0U, 0xA1U, 0xA2U, 0xA3U, 0xA4U, 0xA5U, 0xA6U, 0xA7U};
    std::size_t wrote{};
    std::size_t read_pos{};
    std::uint32_t timeout{};
    bool flushed{};

    [[nodiscard]] arc::Result<std::size_t> write(
        const void* const data,
        const std::size_t bytes) noexcept
    {
        wrote = data == nullptr ? 0U : bytes;
        return wrote;
    }

    [[nodiscard]] arc::Result<std::size_t> read(
        void* const data,
        const std::size_t bytes,
        const std::uint32_t timeout_ms) noexcept
    {
        timeout = timeout_ms;
        auto* out = static_cast<std::uint8_t*>(data);
        const auto available = rx.size() - read_pos;
        const auto count = bytes < available ? bytes : available;
        std::memcpy(out, rx.data() + read_pos, count);
        read_pos += count;
        return count;
    }

    [[nodiscard]] esp_err_t wait(const std::uint32_t timeout_ms) noexcept
    {
        timeout = timeout_ms;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t flush() noexcept
    {
        flushed = true;
        return ESP_OK;
    }

    [[nodiscard]] esp_err_t available(std::size_t& bytes) noexcept
    {
        bytes = rx.size() - read_pos;
        return ESP_OK;
    }
};

struct MockStaticUart {
    [[nodiscard]] static arc::Result<std::size_t> write(const void*, const std::size_t bytes) noexcept
    {
        return bytes;
    }

    [[nodiscard]] static arc::Result<std::size_t> read(
        void*,
        const std::size_t bytes,
        std::uint32_t) noexcept
    {
        return bytes;
    }

    [[nodiscard]] static esp_err_t wait(std::uint32_t) noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t flush() noexcept
    {
        return ESP_OK;
    }

    [[nodiscard]] static esp_err_t available(std::size_t& bytes) noexcept
    {
        bytes = 0U;
        return ESP_OK;
    }
};

static_assert(arc::UartDevice<MockStaticUart>);

void test_any_io()
{
    const arc::AnyOut empty_out;
    expect(!empty_out, "empty AnyOut is false");
    expect(!empty_out.hi().has_value(), "empty AnyOut rejects hi");
    expect(!empty_out.high().has_value(), "empty AnyOut rejects readback");

    MockStaticOut::level = false;
    MockStaticOut::configured = 0U;
    MockStaticOut::writes = 0U;
    const auto static_out = arc::AnyOut::bind<MockStaticOut>();
    expect(static_cast<bool>(static_out), "static AnyOut binds");
    expect(static_out.out().has_value(), "static AnyOut configures");
    expect(static_out.write(true).has_value(), "static AnyOut writes high");
    expect(static_out.high().has_value() && *static_out.high(), "static AnyOut reads high");
    expect(static_out.toggle().has_value(), "static AnyOut toggles");
    expect(static_out.low().has_value() && *static_out.low(), "static AnyOut reads low");
    expect(MockStaticOut::configured == 1U && MockStaticOut::writes == 2U, "static AnyOut thunks");

    MockInput input{.level = true};
    const auto any_in = arc::AnyIn::bind(input);
    expect(static_cast<bool>(any_in), "object AnyIn binds");
    expect(any_in.in().has_value(), "object AnyIn configures");
    expect(any_in.high().has_value() && *any_in.high(), "object AnyIn reads high");
    input.level = false;
    expect(any_in.low().has_value() && *any_in.low(), "object AnyIn reads low");
    expect(input.configured == 1U, "object AnyIn thunks");

    const arc::AnyI2c empty_i2c;
    expect(!empty_i2c, "empty AnyI2c is false");
    expect(empty_i2c.send(nullptr, 1U) == ESP_ERR_INVALID_ARG, "empty AnyI2c rejects send");

    const auto i2c = arc::AnyI2c::bind<MockStaticI2c>();
    expect(static_cast<bool>(i2c), "static AnyI2c binds");
    const std::array<std::uint8_t, 3> tx{0x10U, 0x20U, 0x30U};
    std::array<std::uint8_t, 3> rx{};
    expect(i2c.send(std::span(tx), 77) == ESP_OK, "AnyI2c span send");
    expect(MockStaticI2c::sent == tx.size() && MockStaticI2c::timeout == 77, "AnyI2c send thunks");
    expect(i2c.recv(std::span(rx), 78) == ESP_OK, "AnyI2c span recv");
    expect(rx[0] == 0x5A && rx[2] == 0x5A && MockStaticI2c::received == rx.size(), "AnyI2c recv thunks");
    expect(i2c.xfer(std::span(tx), std::span(rx), 79) == ESP_OK, "AnyI2c span xfer");
    expect(rx[0] == 0xEFU && rx[2] == 0xCFU, "AnyI2c xfer transforms");
    expect(MockStaticI2c::xfer_tx == tx.size() && MockStaticI2c::xfer_rx == rx.size(), "AnyI2c xfer lengths");

    MockSpi mock;
    const auto spi = arc::AnySpi::bind(mock);
    expect(static_cast<bool>(spi), "object AnySpi binds");
    expect(spi.send(std::span(tx), 1'000'000U, 3U) == ESP_OK, "AnySpi span send");
    expect(mock.sent == tx.size() && mock.hz == 1'000'000U && mock.flags == 3U, "AnySpi send thunks");
    rx = {};
    expect(spi.xfer(std::span(tx), std::span(rx), 2'000'000U, 4U) == ESP_OK, "AnySpi span xfer");
    expect(rx == tx && mock.xfer_bytes == tx.size() && mock.hz == 2'000'000U, "AnySpi xfer copies");
    expect(spi.xfer(std::span(tx), std::span(rx).first(2U)) == ESP_ERR_INVALID_ARG, "AnySpi rejects mismatched spans");

    MockUart uart_mock;
    const auto uart = arc::AnyUart::bind(uart_mock);
    expect(static_cast<bool>(uart), "object AnyUart binds");
    const auto wrote = uart.write(std::span(tx));
    expect(wrote.has_value() && *wrote == tx.size() && uart_mock.wrote == tx.size(), "AnyUart writes");
    std::array<std::uint8_t, 4> serial_rx{};
    const auto got = uart.read(std::span(serial_rx), 42U);
    expect(got.has_value() && *got == serial_rx.size(), "AnyUart reads");
    expect(serial_rx[0] == 0xA0U && serial_rx[3] == 0xA3U && uart_mock.timeout == 42U, "AnyUart read thunks");
    std::size_t available{};
    expect(uart.available(available) == ESP_OK && available == 4U, "AnyUart available");
    expect(uart.wait(43U) == ESP_OK && uart_mock.timeout == 43U, "AnyUart wait");
    expect(uart.flush() == ESP_OK && uart_mock.flushed, "AnyUart flush");
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

    const std::array more{5, 6, 7, 8};
    expect(queue.push(std::span(more)) == 3U, "SPSC batch push caps at space");
    expect(queue.size() == 3U, "SPSC batch size");
    expect(queue.space() == 0U, "SPSC batch full space");

    std::array<int, 4> out{};
    expect(queue.pop(std::span(out)) == 3U, "SPSC batch pop available");
    expect(out[0] == 5 && out[1] == 6 && out[2] == 7, "SPSC batch order");
    expect(queue.empty(), "SPSC batch drains empty");

    expect(queue.try_push(9), "SPSC wrap seed");
    int wrap{};
    expect(queue.try_pop(wrap) && wrap == 9, "SPSC wrap seed pop");
    expect(queue.push(std::span(more).subspan(1U, 3U)) == 3U, "SPSC batch wrap push");
    out = {};
    expect(queue.pop(std::span(out).first(2U)) == 2U, "SPSC batch partial pop");
    expect(out[0] == 6 && out[1] == 7, "SPSC batch partial order");
    expect(queue.pop(std::span(out).first(2U)) == 1U && out[0] == 8, "SPSC batch remainder");
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

    expect(fan.try_push<0>(1), "Fanin batch lane 0 first");
    expect(fan.try_push<0>(2), "Fanin batch lane 0 second");
    expect(fan.try_push<1>(10), "Fanin batch lane 1 first");
    expect(fan.try_push<2>(20), "Fanin batch lane 2 first");
    expect(fan.try_push<2>(21), "Fanin batch lane 2 second");
    std::array<int, 6> out{};
    expect(fan.pop(std::span(out)) == 5U, "Fanin batch pop count");
    expect(out[0] == 10 && out[1] == 20 && out[2] == 21 && out[3] == 1 && out[4] == 2, "Fanin batch lane order");
    expect(fan.empty(), "Fanin batch drains empty");

    const std::array lane{100, 101, 102, 103};
    expect(fan.push<1>(std::span(lane)) == 3U, "Fanin batch push caps lane");
    expect(fan.size<1>() == 3U && fan.space<1>() == 0U, "Fanin lane size and space");
    out = {};
    expect(fan.pop(std::span(out)) == 3U, "Fanin batch pushed pop count");
    expect(out[0] == 100 && out[1] == 101 && out[2] == 102, "Fanin batch pushed order");
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

    expect(fan.try_push<0>(30), "Audit Fanin batch lane 0");
    expect(fan.try_push<1>(40), "Audit Fanin batch lane 1");
    std::array<int, 2> out{};
    expect(fan.pop(std::span(out)) == 2U && out[0] == 30 && out[1] == 40, "Audit Fanin batch pop");

    const std::array lane{50, 51, 52};
    expect(fan.push<0>(std::span(lane)) == 3U, "Audit Fanin batch push");
    expect(fan.size<0>() == 3U && fan.space<0>() == 0U, "Audit Fanin lane size and space");
    expect(fan.pop(std::span(out).first(2U)) == 2U && out[0] == 50 && out[1] == 51, "Audit Fanin partial batch pop");
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

    const std::array<std::uint8_t, 4> overflow_option{0xe0U, 0xffU, 0xf2U, 0x00U};
    std::size_t overflow_offset{};
    std::uint16_t overflow_number = 65530U;
    const auto overflow = arc::net::Coap::next(overflow_option, overflow_offset, overflow_number);
    expect(!overflow.has_value(), "CoAP option number overflow rejects");
}

void test_invalid_codecs()
{
    const std::array<std::uint8_t, 2> reserved_ws_opcode{0x83U, 0x00U};
    expect(!arc::net::Ws::parse(reserved_ws_opcode), "WS reserved opcode rejects");

    std::array<std::uint8_t, 8> ws_buffer{};
    expect(!arc::net::Ws::frame(ws_buffer, static_cast<arc::net::WsOpcode>(0x3U), nullptr, 0U),
           "WS reserved opcode encode rejects");

    const std::array<std::uint8_t, 7> mqtt_reserved_qos{0x36U, 0x05U, 0x00U, 0x01U, 't', 0x00U, 0x01U};
    expect(!arc::net::Mqtt::parse(mqtt_reserved_qos), "MQTT reserved publish QoS rejects");

    const std::array<std::uint8_t, 2> mqtt_reserved_type{0xf0U, 0x00U};
    expect(!arc::net::Mqtt::parse(mqtt_reserved_type), "MQTT reserved type rejects");

    const std::array<std::uint8_t, 2> mqtt_bad_subscribe_flags{0x80U, 0x00U};
    expect(!arc::net::Mqtt::parse(mqtt_bad_subscribe_flags), "MQTT fixed flags reject");
}

void test_caps()
{
    struct alignas(64) OverAligned {
        std::uint32_t value{};
    };

    auto ptr = arc::caps<OverAligned, MALLOC_CAP_INTERNAL>();
    expect(static_cast<bool>(ptr), "caps over-aligned allocation succeeds");
    expect((reinterpret_cast<std::uintptr_t>(ptr.get()) & (alignof(OverAligned) - 1U)) == 0U,
           "caps allocation preserves over-alignment");

    heap_caps_last_calloc_bytes = 0U;
    auto plain = arc::inbuf<std::uint8_t>(65U);
    expect(static_cast<bool>(plain), "plain cap buffer allocation succeeds");
    expect(plain.size() == 65U && plain.bytes() == 65U, "plain cap buffer preserves logical size");
    expect(heap_caps_last_calloc_bytes == 65U, "plain cap buffer allocation stays logical");

    heap_caps_last_calloc_bytes = 0U;
    auto dma = arc::dmabuf<std::uint8_t>(65U);
    const auto padded = (65U + arc::cache_line - 1U) & ~(arc::cache_line - 1U);
    expect(static_cast<bool>(dma), "DMA cap buffer allocation succeeds");
    expect(dma.size() == 65U && dma.bytes() == 65U, "DMA cap buffer preserves logical size");
    expect((reinterpret_cast<std::uintptr_t>(dma.data()) & (arc::cache_line - 1U)) == 0U,
           "DMA cap buffer preserves cache-line alignment");
    expect(heap_caps_last_calloc_bytes == padded, "DMA cap buffer allocates whole cache lines");
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

void test_stream()
{
    struct Mock {
        std::array<std::uint8_t, 32> tx{};
        std::array<std::uint8_t, 32> rx{};
        std::size_t tx_pos{};
        std::size_t rx_pos{};
        std::size_t rx_len{};

        arc::Status send_all(const void* const data, const std::size_t bytes) noexcept
        {
            if (tx_pos + bytes > tx.size()) {
                return arc::Status{arc::fail(ESP_ERR_NO_MEM)};
            }
            std::memcpy(tx.data() + tx_pos, data, bytes);
            tx_pos += bytes;
            return arc::ok();
        }

        arc::Result<std::size_t> recv(void* const data, const std::size_t bytes) noexcept
        {
            if (rx_pos >= rx_len) {
                return std::size_t{};
            }

            const auto n = bytes < 2U ? bytes : 2U;
            const auto left = rx_len - rx_pos;
            const auto got = n < left ? n : left;
            std::memcpy(data, rx.data() + rx_pos, got);
            rx_pos += got;
            return got;
        }
    };

    static_assert(arc::net::ByteStream<Mock>);

    Mock io{};
    const std::array<std::uint8_t, 3> payload{1U, 2U, 3U};
    expect(arc::net::Stream::write_frame16(io, std::span(payload)).has_value(), "Stream frame write");
    expect(io.tx_pos == 5U, "Stream frame bytes");
    expect(io.tx[0] == 0U && io.tx[1] == 3U && io.tx[4] == 3U, "Stream frame layout");
    expect(arc::net::Stream::write_frame16(io, nullptr, 0U).has_value(), "Stream empty frame write");
    expect(io.tx_pos == 7U && io.tx[5] == 0U && io.tx[6] == 0U, "Stream empty frame layout");

    std::array<std::uint8_t, 8> encoded{};
    const auto coded = arc::net::Stream::frame16(std::span(encoded), std::span(payload));
    expect(coded.has_value() && coded->size() == 5U, "Stream frame encode");
    expect((*coded)[0] == 0U && (*coded)[1] == 3U && (*coded)[4] == 3U, "Stream frame encode layout");

    io.rx = {0U, 3U, 7U, 8U, 9U};
    io.rx_len = 5U;
    std::array<std::uint8_t, 4> out{};
    const auto frame = arc::net::Stream::read_frame16(io, std::span(out));
    expect(frame.has_value() && *frame == 3U, "Stream frame read");
    expect((out == std::array<std::uint8_t, 4>{7U, 8U, 9U, 0U}), "Stream frame payload");

    io.rx = {0U, 5U, 1U, 2U, 3U, 4U, 5U};
    io.rx_pos = 0U;
    io.rx_len = 7U;
    expect(!arc::net::Stream::read_frame16(io, std::span(out)), "Stream oversized frame rejects");
}

void test_refinit()
{
    std::uint32_t state{};
    expect(arc::Init::is_empty(state), "Init starts empty");
    expect(!arc::Init::is_busy(state), "Init not busy initially");
    expect(!arc::Init::is_ready(state), "Init not ready initially");

    {
        arc::TryGate first(state);
        expect(static_cast<bool>(first), "TryGate takes open gate");

        arc::TryGate second(state);
        expect(!static_cast<bool>(second), "TryGate rejects shut gate");

        auto moved = std::move(first);
        expect(!static_cast<bool>(first), "TryGate move clears source");
        expect(static_cast<bool>(moved), "TryGate move keeps destination");
        moved.reset();
    }
    expect(arc::Gate::try_take(state), "TryGate reset opens gate");
    arc::Gate::drop(state);

    {
        auto txn = arc::InitTxn::begin(state);
        expect(static_cast<bool>(txn), "InitTxn first begin initializes");
        expect(arc::Init::is_busy(state), "InitTxn begin marks busy");
    }
    expect(arc::Init::is_empty(state), "InitTxn destructor restores empty");
    expect(arc::InitTxn::begin(state).pass(), "InitTxn destructor rolls back busy state");
    expect(arc::Init::is_ready(state), "InitTxn pass marks ready");
    expect(!arc::InitTxn::begin(state), "InitTxn ready state does not initialize");
    expect(arc::Init::take(state), "InitTxn pass leaves ready state");
    expect(arc::Init::is_busy(state), "Init take marks busy");
    arc::Init::fail(state);
    expect(arc::Init::is_empty(state), "Init fail restores empty");

    auto txn = arc::InitTxn::begin(state);
    expect(static_cast<bool>(txn), "InitTxn move source active");
    auto moved = std::move(txn);
    expect(!static_cast<bool>(txn), "InitTxn move clears source");
    expect(static_cast<bool>(moved), "InitTxn move keeps destination");
    expect(moved.fail(), "InitTxn explicit fail");

    {
        auto ref_txn = arc::RefInitTxn::begin(state);
        expect(static_cast<bool>(ref_txn), "RefInitTxn first begin initializes");
        expect(arc::RefInit::is_busy(state), "RefInitTxn begin marks busy");
    }
    expect(arc::RefInit::is_empty(state), "RefInitTxn destructor restores empty");
    expect(arc::RefInitTxn::begin(state).pass(), "RefInitTxn destructor rolls back busy state");
    expect(arc::RefInit::is_ready(state), "RefInitTxn pass marks ready");
    expect(arc::RefInit::refs(state) == 1U, "RefInitTxn pass owns first ref");
    expect(!arc::RefInitTxn::begin(state), "RefInitTxn ready state shares without init");
    expect(arc::RefInit::refs(state) == 2U, "RefInitTxn ready begin increments refs");
    expect(!arc::RefInit::drop(state), "RefInitTxn shared drop");
    expect(arc::RefInit::drop(state), "RefInitTxn final drop");
    arc::RefInit::fail(state);

    auto ref_txn = arc::RefInitTxn::begin(state);
    expect(static_cast<bool>(ref_txn), "RefInitTxn move source active");
    auto moved_ref = std::move(ref_txn);
    expect(!static_cast<bool>(ref_txn), "RefInitTxn move clears source");
    expect(static_cast<bool>(moved_ref), "RefInitTxn move keeps destination");
    expect(moved_ref.fail(), "RefInitTxn explicit fail");

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
    test_any_io();
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
    test_invalid_codecs();
    test_caps();
    test_dsp();
    test_seqreg();
    test_file();
    test_stream();
    test_refinit();
    std::puts("arc host tests: OK");
}
